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
#include <thread>

#include "simple_log.h"
#include "actuator_subscriber.h"

// declare logger in root namespace
LOGGER_STATIC_INIT("# ActuatorSubscriber::");

namespace sdv {
namespace broker_feeder {
namespace kuksa {

// helper for printing vector elements
template <typename T> std::ostream& operator<<(std::ostream& os, const std::vector<T> &v) {
    for (auto t : v) {
        os << t << " ";
    }
    return os;
}

ActuatorSubscriber::ActuatorSubscriber(std::shared_ptr<CollectorClient> client) :
    client_(client),
    subscriber_active_(false),
    cb_(nullptr),
    actuators_()
{
    LOGGER_SET_LEVEL_ENV("KUKSA_DEBUG", LEVEL_INF);
    // subscriber_context_ = client_->createClientContext();
}

ActuatorSubscriber::~ActuatorSubscriber() {
    LOG_TRACE << "called." << std::endl;
    Shutdown();
    LOG_TRACE << "done." << std::endl;
}

void ActuatorSubscriber::Shutdown() {
    LOG_DEBUG << "subscriber_active_:" << subscriber_active_ << std::endl;
    if (subscriber_active_) {
        subscriber_active_ = false;
        if (subscriber_context_) {
            LOG_DEBUG << "Cancelling subscriber context ..." << std::endl;
            std::lock_guard<std::mutex> lock(context_mutex_);
            LOG_TRACE << "context->TryCancel()" << std::endl;
            subscriber_context_->TryCancel();
        }
    }
}

void ActuatorSubscriber::Init(std::vector<std::string> subscribe_actuators, ActuatorChangeCallback cb) {
    actuators_ = subscribe_actuators;
    cb_ = cb;
    if (LOGGER_ENABLED(LEVEL_INF)) {
        std::stringstream ss;
        ss << "Initialized for actuators: [ ";
        for (auto sub: subscribe_actuators) {
            ss << sub << " ";
        }
        ss << "]" << std::endl;
        LOG_INFO << ss.str();
    }
}

void ActuatorSubscriber::Run() {

    if (actuators_.size() == 0) {
        LOG_ERROR << "No actuators to subscribe!";
        subscriber_active_ = false;
        LOG_INFO << "Exiting" << std::endl;
        return;
    }

    subscriber_active_ = true;
    int backoff = 1;
    LOG_INFO << "Starting actuator target subscriber [" << client_->GetBrokerAddr() << "]" << std::endl;
    while (subscriber_active_) {
        auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(backoff);
        if (!client_->WaitForConnected(deadline)) {
            LOG_INFO << "Not connected" << std::endl;
            if (backoff < 10) {
                backoff++;
            }
            continue;
        } else {
            backoff = 1;
        }

        LOG_INFO << "Connected to [" << client_->GetBrokerAddr() << "]" << std::endl;

        ::kuksa::val::v1::SubscribeRequest request;
        for (auto i=0; i<actuators_.size(); i++) {
            auto entry = request.add_entries();
            entry->set_path(actuators_[i]);
            entry->add_fields(::kuksa::val::v1::Field::FIELD_ACTUATOR_TARGET);
            entry->add_fields(::kuksa::val::v1::Field::FIELD_METADATA);
        }

        ::kuksa::val::v1::SubscribeResponse response;
        subscriber_context_ = client_->createClientContext();
        if (LOGGER_ENABLED(LEVEL_DBG)) {
            { LOG_DEBUG << "Subscribing: [ " << actuators_ << "]" << std::endl; }
        }
        std::unique_ptr<::grpc::ClientReader<::kuksa::val::v1::SubscribeResponse>> reader(
            client_->Subscribe(subscriber_context_.get(), request));

        LOG_INFO << "Actuator targets Subscribed!" << std::endl;
        while (subscriber_active_ && reader->Read(&response)) {
            LOG_TRACE << "[SUB] updates_size:" << response.updates_size() << std::endl;
            // std::pair<std::string, int> entries;
            ActuatorValues changes;
            for (auto& update : response.updates()) {
                if (!subscriber_active_) break;
                if (LOGGER_ENABLED(LEVEL_DBG)) {
                    std::stringstream ss;
                    ss  << "[SUB] " << update.entry().path()
                        << ", value_case:" << static_cast<int>(update.entry().actuator_target().value_case())
                        << ", target: { " << update.entry().actuator_target().ShortDebugString() << " }";
                    LOG_DEBUG << ss.str() << std::endl;
                }
                // DatapointEntry dpe;
                // dpe.name = update.entry().path();
                // dpe.value = update.entry().actuator_target();
                changes.emplace(
                    update.entry().path(),
                    update.entry().actuator_target());
            }
            if (cb_ && subscriber_active_) {
                cb_(changes);
            }
        }
        grpc::Status status = reader->Finish();
        if (status.ok()) {
            LOG_INFO << "Disconnected." << std::endl;
        } else {
            LOG_ERROR << "Disconnected with status: "
                    << subscriber_context_->debug_error_string() << std::endl;
            if (subscriber_active_) {
                client_->handleGrpcError(status, "ActuatorSubscriber::Run()");
                // prevent busy polling if subscribe failed with error
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
        {
            // TODO: lock mutex
            std::lock_guard<std::mutex> lock(context_mutex_);
            subscriber_context_ = nullptr;
        }
    }

    LOG_DEBUG << "Exiting" << std::endl;
}

std::shared_ptr<ActuatorSubscriber> ActuatorSubscriber::createInstance(std::shared_ptr<CollectorClient> client) {
    return std::make_shared<ActuatorSubscriber>(client);
}


}  // namespace kuksa
}  // namespace broker_feeder
}  // namespace sdv
