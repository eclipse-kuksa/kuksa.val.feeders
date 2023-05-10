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

#include <atomic>
#include <map>
#include <mutex>
#include <string>

#include "sdv/databroker/v1/types.pb.h"
#include "collector_client.h"

namespace sdv {
namespace broker_feeder {
namespace kuksa {

typedef std::map<std::string, ::kuksa::val::v1::Datapoint> ActuatorValues;

typedef std::function<void(ActuatorValues target_values)> ActuatorChangeCallback;

class ActuatorSubscriber {
public:

    ActuatorSubscriber(std::shared_ptr<CollectorClient> client);
    virtual ~ActuatorSubscriber();

    void Init(std::vector<std::string> actuators = {}, ActuatorChangeCallback cb = nullptr);
    void Run();
    void Shutdown();
    static std::shared_ptr<ActuatorSubscriber> createInstance(std::shared_ptr<CollectorClient> client);

protected:
    // disable copy constructor and copy assignment
    ActuatorSubscriber() = default;
    ActuatorSubscriber(const ActuatorSubscriber&) = delete;
    ActuatorSubscriber& operator=(const ActuatorSubscriber&) = delete;

private:
    int log_level_;
    std::shared_ptr<CollectorClient> client_;
    std::unique_ptr<grpc::ClientContext> subscriber_context_;
    std::atomic<bool> subscriber_active_;
    std::mutex context_mutex_;
    std::vector<std::string> actuators_;
    ActuatorChangeCallback cb_;
};


}  // namespace kuksa
}  // namespace broker_feeder
}  // namespace sdv
