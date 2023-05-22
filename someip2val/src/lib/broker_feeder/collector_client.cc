/********************************************************************************
* Copyright (c) 2022 Contributors to the Eclipse Foundation
*
* See the NOTICE file(s) distributed with this work for additional
* information regarding copyright ownership.
*
* This program and the accompanying materials are made available under the
* terms of the Apache License 2.0 which is available at
* http://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
********************************************************************************/

#include <sstream>

#include "simple_log.h"
#include "collector_client.h"

/*** LOG helpers */
LOGGER_STATIC_INIT("# CollectorClient::");

namespace sdv {
namespace broker_feeder {


std::string getEnvVar(const std::string& name, const std::string& defaultValue) {
    char* value = std::getenv(name.c_str());
    return value != nullptr ? std::string(value) : defaultValue;
}

std::shared_ptr<CollectorClient> CollectorClient::createInstance(std::string broker_addr, std::string auth_token) {
    return std::make_shared<CollectorClient>(broker_addr, auth_token);
}

CollectorClient::CollectorClient(std::string broker_addr, std::string auth_token)
    : broker_addr_(broker_addr)
    , auth_token_(auth_token)
    , connected_(false)
{
    // set log level from env "KUKSA_DEBUG", defaults to info
    LOGGER_SET_LEVEL_ENV("KUKSA_DEBUG", LEVEL_INF);

    changeToDaprPortIfSet(broker_addr);
    metadata_ = getGrpcMetadata();
    channel_ = grpc::CreateChannel(broker_addr, grpc::InsecureChannelCredentials());
    stub_ = sdv::databroker::v1::Collector::NewStub(channel_);
    kuksa_stub_ = kuksa::val::v1::VAL::NewStub(channel_);
}

bool CollectorClient::WaitForConnected(std::chrono::_V2::system_clock::time_point deadline) {
    connected_ = channel_->WaitForConnected(deadline);
    return connected_;
}

grpc_connectivity_state CollectorClient::GetState() {
    return channel_->GetState(false);
}

bool CollectorClient::Connected() {
    return connected_;
}

void CollectorClient::SetDisconnected() {
    connected_ = false;
}

std::string CollectorClient::GetBrokerAddr() {
    return broker_addr_;
}

GrpcMetadata CollectorClient::getGrpcMetadata() {
    GrpcMetadata grpc_metadata;
    std::string dapr_app_id = getEnvVar("VEHICLEDATABROKER_DAPR_APP_ID");
    if (!dapr_app_id.empty()) {
        grpc_metadata["dapr-app-id"] = dapr_app_id;
        LOG_TRACE << "Setting dapr-app-id: " << dapr_app_id << std::endl;

    }
    // allow overriding token file from env
    std::string databroker_token = auth_token_.empty() ? getEnvVar("BROKER_TOKEN") : auth_token_;
    if (!databroker_token.empty()) {
        auto header = "Bearer " + databroker_token;
        grpc_metadata["authorization"] = header;
        LOG_TRACE << "Adding authorization: " << header << std::endl;
    }
    return grpc_metadata;
}


/** Change the port of the broker address passed to the c-tor to the port
 *  set by a possibly set DAPR_GRPC_PORT environment variable. */
void CollectorClient::changeToDaprPortIfSet(std::string& broker_addr) {
    std::string dapr_port = getEnvVar("DAPR_GRPC_PORT");
    if (!dapr_port.empty()) {
        std::string::size_type colon_pos = broker_addr.find_last_of(':');
        broker_addr = broker_addr.substr(0, colon_pos + 1) + dapr_port;
        LOG_INFO << "changing to DAPR GRPC port:" << broker_addr << std::endl;
    }
}

::grpc::Status CollectorClient::RegisterDatapoints(::grpc::ClientContext* context,
                                                   const ::sdv::databroker::v1::RegisterDatapointsRequest& request,
                                                   ::sdv::databroker::v1::RegisterDatapointsReply* response) {
    return stub_->RegisterDatapoints(context, request, response);
}

::grpc::Status CollectorClient::UpdateDatapoints(::grpc::ClientContext* context,
                                                 const ::sdv::databroker::v1::UpdateDatapointsRequest& request,
                                                 ::sdv::databroker::v1::UpdateDatapointsReply* response) {
    return stub_->UpdateDatapoints(context, request, response);
}

std::unique_ptr<::grpc::ClientReader<::kuksa::val::v1::SubscribeResponse>>
CollectorClient::Subscribe(::grpc::ClientContext *context,
                           const ::kuksa::val::v1::SubscribeRequest &request) {
    return kuksa_stub_->Subscribe(context, request);
}

/** Create the client context for a gRPC call and add possible gRPC metadata */
std::unique_ptr<grpc::ClientContext> CollectorClient::createClientContext() {
    auto context = std::make_unique<grpc::ClientContext>();
    for (const auto& metadata : metadata_) {
        context->AddMetadata(metadata.first, metadata.second);
        LOG_TRACE << "  AddMetadata(" << metadata.first << ", " << metadata.second << ")" << std::endl;
    }
    LOG_TRACE << "ClientContext created." << std::endl;
    return context;
}

bool CollectorClient::handleGrpcError(const grpc::Status& status, const std::string& caller) {
    if (status.ok()) return false;

    bool fatal_error = true;
    std::stringstream ss;
    ss  << caller << " failed:\n"
        << "    ErrorCode: "  << status.error_code() << "\n"
        << "    ErrorMsg:  '" << status.error_message() << "'\n"
        << "    ErrorDet:  '" << status.error_details() << "'\n"
        << "    grpcChannelState: " << GetState();
    LOG_ERROR << ss.str() << std::endl;

    switch (status.error_code()) {
        case GRPC_STATUS_INTERNAL:
        case GRPC_STATUS_UNAUTHENTICATED:
        case GRPC_STATUS_UNIMPLEMENTED:
        // case GRPC_STATUS_UNKNOWN: // disabled due to dapr {GRPC_STATUS_UNKNOWN; ErrorMsg: 'timeout waiting for address for app id vehicledatabroker'}
            LOG_ERROR << ">>> Unrecoverable error -> stopping client." << std::endl;
            fatal_error = true;
            break;
        default:
            LOG_ERROR << ">>> Maybe temporary error -> trying reconnection to broker" << std::endl;
            fatal_error = false;
            break;
    }
    SetDisconnected();
    return fatal_error;
}

}  // namespace broker_feeder
}  // namespace sdv
