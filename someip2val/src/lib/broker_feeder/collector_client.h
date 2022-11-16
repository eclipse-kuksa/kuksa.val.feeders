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

//// NOTE: uncomment line below to include experimental collector subscribe actuator support
#define DATABROKER_SUBSCRIBE_ACTUATOR

#include "sdv/databroker/v1/collector.grpc.pb.h"


namespace sdv {
namespace broker_feeder {

using GrpcMetadata = std::map<std::string, std::string>;


class CollectorClient {
   public:

    /**
     * Create a new instance
     *
     * @param broker_addr address of the broker to connect to; format "<ip-address>:<port>"
     */
    static std::shared_ptr<CollectorClient> createInstance(const std::string &broker_addr);

    CollectorClient(std::string broker_addr);

    bool WaitForConnected(std::chrono::_V2::system_clock::time_point deadline);

    grpc_connectivity_state GetState();
    bool Connected();

    void SetDisconnected();

    std::string GetBrokerAddr();

    /** Change the port of the broker address passed to the c-tor to the port
     *  set by a possibly set DAPR_GRPC_PORT environment variable. */
    void changeToDaprPortIfSet(std::string& broker_addr);

    ::grpc::Status RegisterDatapoints(::grpc::ClientContext* context,
                                      const ::sdv::databroker::v1::RegisterDatapointsRequest& request,
                                      ::sdv::databroker::v1::RegisterDatapointsReply* response);

    ::grpc::Status UpdateDatapoints(::grpc::ClientContext* context,
                                    const ::sdv::databroker::v1::UpdateDatapointsRequest& request,
                                    ::sdv::databroker::v1::UpdateDatapointsReply* response);
#ifdef DATABROKER_SUBSCRIBE_ACTUATOR
    std::unique_ptr<::grpc::ClientReader<::sdv::databroker::v1::SubscribeActuatorTargetReply>> SubscribeActuatorTargets(
        ::grpc::ClientContext* context, const ::sdv::databroker::v1::SubscribeActuatorTargetRequest& request);
#endif

    std::unique_ptr<grpc::ClientContext> createClientContext();

   private:

    GrpcMetadata metadata_;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<sdv::databroker::v1::Collector::Stub> stub_;
    std::atomic<bool> connected_;

    std::string broker_addr_;
};



}
}
