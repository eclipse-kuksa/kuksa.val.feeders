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
#pragma once

#include <grpcpp/grpcpp.h>

#include <atomic>
#include <memory>

#include "sdv/databroker/v1/collector.grpc.pb.h"
#include "kuksa/val/v1/val.grpc.pb.h"

namespace sdv {
namespace broker_feeder {

using GrpcMetadata = std::map<std::string, std::string>;

std::string getEnvVar(const std::string& name, const std::string& defaultValue = {});

class CollectorClient {

public:

    /**
     * Create a new instance
     *
     * @param broker_addr address of the broker to connect to; format "<ip-address>:<port>"
     * @param auth_token Optional OAuth jwt token to authorize with DataBroker
     */
    static std::shared_ptr<CollectorClient> createInstance(std::string broker_addr, std::string auth_token = {});

    /**
     * @brief Construct a new Collector Client object
     *
     * @param broker_addr address of the broker to connect to; format "<ip-address>:<port>"
     * @param auth_token Optional OAuth jwt token to authorize with DataBroker
     */
    CollectorClient(std::string broker_addr, std::string auth_token);

    virtual ~CollectorClient() = default;

    bool WaitForConnected(std::chrono::_V2::system_clock::time_point deadline);

    grpc_connectivity_state GetState();

    bool Connected();

    void SetDisconnected();

    std::string GetBrokerAddr();

    std::unique_ptr<grpc::ClientContext> createClientContext();

    ::grpc::Status RegisterDatapoints(::grpc::ClientContext* context,
                const ::sdv::databroker::v1::RegisterDatapointsRequest& request,
                ::sdv::databroker::v1::RegisterDatapointsReply* response);

    ::grpc::Status UpdateDatapoints(::grpc::ClientContext* context,
                const ::sdv::databroker::v1::UpdateDatapointsRequest& request,
                ::sdv::databroker::v1::UpdateDatapointsReply* response);

	std::unique_ptr<::grpc::ClientReader<::kuksa::val::v1::SubscribeResponse>> Subscribe(
				::grpc::ClientContext *context,
                const ::kuksa::val::v1::SubscribeRequest &request);

    /** Log the gRPC error information and
     *   - either trigger re-connection and "recoverable" errors
     *   - or deactivate the feeder.
     */
    bool handleGrpcError(const grpc::Status& status, const std::string& caller);

protected:
    /** Change the port of the broker address passed to the c-tor to the port
     *  set by a possibly set DAPR_GRPC_PORT environment variable. */
    void changeToDaprPortIfSet(std::string& broker_addr);

    /**
     * @brief Returns GRPC Metadata object with set "authorization" / "dapr-app-id" headers
     *
     * @return GrpcMetadata
     */
    GrpcMetadata getGrpcMetadata();

private:
    GrpcMetadata metadata_;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<sdv::databroker::v1::Collector::Stub> stub_;
    std::unique_ptr<kuksa::val::v1::VAL::Stub> kuksa_stub_;
    std::atomic<bool> connected_;
    std::atomic<bool> active_;
    std::string broker_addr_;
    std::string auth_token_;
};


}  // namespace broker_feeder
}  // namespace sdv
