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

#include <csignal>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h> // access()

#include "someip_kuksa_adapter.h"
#include "data_broker_feeder.h"
#include "create_datapoint.h"

#include "someip_client.h"
#include "wiper_poc.h"

using sdv::databroker::v1::Datapoint;
using sdv::databroker::v1::DataType;
using sdv::databroker::v1::ChangeType;
using sdv::databroker::v1::Datapoint_Failure;


namespace sdv {
namespace adapter {

/*** LOG helpers */
#define LEVEL_TRC   3
#define LEVEL_DBG   2
#define LEVEL_INF   1
#define LEVEL_ERR   0
#define MODULE_PREFIX   "# SomeipFeederAdapter::"

#define LOG_TRACE   if (log_level_ >= LEVEL_TRC) std::cout << MODULE_PREFIX << __func__ << ": [trace] "
#define LOG_DEBUG   if (log_level_ >= LEVEL_DBG) std::cout << MODULE_PREFIX << __func__ << ": [debug] "
#define LOG_INFO    if (log_level_ >= LEVEL_INF) std::cout << MODULE_PREFIX << __func__ << ": [info] "
#define LOG_ERROR   if (log_level_ >= LEVEL_ERR) std::cerr << MODULE_PREFIX << __func__ << ": [error] "

/*** VSS Paths for WIPER */
const std::string WIPER_MODE = WIPER_VSS_PATH + ".Mode";
const std::string WIPER_FREQUENCY = WIPER_VSS_PATH + ".Frequency";
const std::string WIPER_TARGET_POSITION = WIPER_VSS_PATH + ".TargetPosition";
const std::string WIPER_DRIVE_CURRENT = WIPER_VSS_PATH + ".DriveCurrent";
const std::string WIPER_ACTUAL_POSITION = WIPER_VSS_PATH + ".ActualPosition";
const std::string WIPER_IS_WIPING = WIPER_VSS_PATH + ".IsWiping";
const std::string WIPER_IS_ENDING_WIPE_CYCLE = WIPER_VSS_PATH + ".IsEndingWipeCycle";
const std::string WIPER_IS_WIPER_ERROR = WIPER_VSS_PATH + ".IsWiperError";
const std::string WIPER_IS_POSITION_REACHED = WIPER_VSS_PATH + ".IsPositionReached";
const std::string WIPER_IS_BLOCKED = WIPER_VSS_PATH + ".IsBlocked";
const std::string WIPER_IS_OVERHEATED = WIPER_VSS_PATH + ".IsOverheated";


SomeipFeederAdapter::SomeipFeederAdapter():
    feeder_active_(false),
    databroker_addr_(),
    databroker_feeder_(nullptr),
    feeder_thread_(nullptr),
    actuator_target_subscriber_thread_(nullptr),
    actuator_target_subscriber_running_(false),
    actuator_target_subscriber_context_(nullptr),
    someip_active_(false),
    someip_thread_(nullptr),
    someip_client_(nullptr),
    someip_use_tcp_(false),
    shutdown_requested_(false)
{
    log_level_ = sdv::someip::getEnvironmentInt("SOMEIP_CLI_DEBUG", 1);
}

SomeipFeederAdapter::~SomeipFeederAdapter() {
    LOG_TRACE << "called." << std::endl;
    Shutdown();
    LOG_TRACE << "done." << std::endl;
}

bool SomeipFeederAdapter::InitDataBrokerFeeder(const std::string &databroker_addr) {

    sdv::broker_feeder::DatapointConfiguration metadata = {
        {WIPER_MODE,
            DataType::STRING,
            ChangeType::ON_CHANGE,
            sdv::broker_feeder::createNotAvailableValue(),
            "Requested mode of wiper system. ['STOP_HOLD', 'WIPE', 'PLANT_MODE', 'EMERGENCY_STOP']"
        },
        {WIPER_FREQUENCY,
            DataType::UINT8,
            ChangeType::ON_CHANGE,
            sdv::broker_feeder::createNotAvailableValue(),
            "Wiping frequency/speed, measured in cycles per minute."
        },
        {WIPER_TARGET_POSITION,
            DataType::FLOAT,
            ChangeType::ON_CHANGE,
            sdv::broker_feeder::createNotAvailableValue(),
            "Requested position of main wiper blade for the wiper system relative to reference position."
        },
        {WIPER_ACTUAL_POSITION,
            DataType::FLOAT,
            ChangeType::ON_CHANGE,
            sdv::broker_feeder::createNotAvailableValue(),
            "Actual position of main wiper blade for the wiper system relative to reference position."
        },
        {WIPER_DRIVE_CURRENT,
            DataType::FLOAT,
            ChangeType::ON_CHANGE,
            sdv::broker_feeder::createNotAvailableValue(),
            "Actual current used by wiper drive."
        },
        {WIPER_IS_WIPING,
            DataType::BOOL,
            ChangeType::ON_CHANGE,
            sdv::broker_feeder::createNotAvailableValue(),
            "True if wiper blades are moving."
        },
        {WIPER_IS_ENDING_WIPE_CYCLE,
            DataType::BOOL,
            ChangeType::ON_CHANGE,
            sdv::broker_feeder::createNotAvailableValue(),
            "Indicates if current wipe movement is completed or near completion."
        },
        {WIPER_IS_WIPER_ERROR,
            DataType::BOOL,
            ChangeType::ON_CHANGE,
            sdv::broker_feeder::createNotAvailableValue(),
            "Indicates system failure."
        },
        {WIPER_IS_POSITION_REACHED,
            DataType::BOOL,
            ChangeType::ON_CHANGE,
            sdv::broker_feeder::createNotAvailableValue(),
            "Indicates if a requested position has been reached."
        },
        {WIPER_IS_BLOCKED,
            DataType::BOOL,
            ChangeType::ON_CHANGE,
            sdv::broker_feeder::createNotAvailableValue(),
            "Indicates if wiper movement is blocked."
        },
        {WIPER_IS_OVERHEATED,
            DataType::BOOL,
            ChangeType::ON_CHANGE,
            sdv::broker_feeder::createNotAvailableValue(),
            "Indicates if wiper system is overheated."
            // NOTE: evaluate someip event: TempGear and ECUTemp
        }
    };

    LOG_INFO << "Connecting to " << databroker_addr << std::endl;
    collector_client_ = sdv::broker_feeder::CollectorClient::createInstance(databroker_addr);
    databroker_feeder_ = sdv::broker_feeder::DataBrokerFeeder::createInstance(collector_client_, std::move(metadata));
    return true;
}

bool SomeipFeederAdapter::InitSomeipClient(sdv::someip::SomeIPConfig _config) {

    someip_use_tcp_ = _config.use_tcp;

    auto someip_app = ::getenv("VSOMEIP_APPLICATION_NAME");
    auto someip_config = ::getenv("VSOMEIP_CONFIGURATION");

    bool someip_ok = true;
    if (!someip_app) {
        LOG_ERROR << "VSOMEIP_APPLICATION_NAME not set in environment, someip disabled!" << std::endl;
        someip_ok = false;
    }
    if (someip_config == nullptr) {
        LOG_ERROR << "VSOMEIP_CONFIGURATION not set in environment, someip disabled!" << std::endl;
        someip_ok = false;
    } else {
        if (::access(someip_config, F_OK) == -1) {
            LOG_ERROR << "env VSOMEIP_CONFIGURATION file is missing: " << someip_config << std::endl;
            someip_ok = false;
        }
    }

    if (someip_ok) {
        LOG_INFO << std::endl;
        LOG_INFO << "### VSOMEIP_APPLICATION_NAME=" << someip_app << std::endl;
        LOG_INFO << "### VSOMEIP_CONFIGURATION=" << someip_config << std::endl;
        std::cout << "$ cat " << someip_config << std::endl;
        std::string cmd;
        cmd = "cat " + std::string(someip_config);
        int rc = ::system(cmd.c_str());
        std::cout << std::endl;


        someip_client_ = sdv::someip::SomeIPClient::createInstance(
            _config,
            std::bind(&SomeipFeederAdapter::on_someip_message,
                    this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
    }
    someip_active_ = someip_ok;
    return someip_ok;
}

void SomeipFeederAdapter::RunActuatorTargetSubscriber() {
    LOG_INFO << "Starting actuator target subscriber..." << std::endl;

    actuator_target_subscriber_running_ = true;
    int backoff = 1;
    while (actuator_target_subscriber_running_) {
        auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(backoff);
        if (!collector_client_->WaitForConnected(deadline)) {
            LOG_INFO << "Not connected" << std::endl;
            if (backoff < 10) {
                backoff++;
            }
            continue;
        } else {
            backoff = 1;
        }

        LOG_INFO << "Connected" << std::endl;

        sdv::databroker::v1::SubscribeActuatorTargetRequest request;
        request.add_paths(WIPER_MODE);
        request.add_paths(WIPER_FREQUENCY);
        request.add_paths(WIPER_TARGET_POSITION);

        sdv::databroker::v1::SubscribeActuatorTargetReply reply;
        actuator_target_subscriber_context_ = collector_client_->createClientContext();
        std::unique_ptr<::grpc::ClientReader<sdv::databroker::v1::SubscribeActuatorTargetReply>> reader(
            collector_client_->SubscribeActuatorTargets(actuator_target_subscriber_context_.get(), request));
        while (reader->Read(&reply)) {
            auto actuator_targets = reply.actuator_targets();
            if (actuator_targets.contains(WIPER_MODE) &&
                actuator_targets.contains(WIPER_FREQUENCY) &&
                actuator_targets.contains(WIPER_TARGET_POSITION)) {

                auto wiper_mode = actuator_targets.at(WIPER_MODE);
                if (wiper_mode.value_case() != sdv::databroker::v1::Datapoint::ValueCase::kStringValue) {
                    LOG_INFO << "wrong value type for " << WIPER_MODE << std::endl;
                    continue;
                }
                auto wiper_freq = actuator_targets.at(WIPER_FREQUENCY);
                if (wiper_freq.value_case() != sdv::databroker::v1::Datapoint::ValueCase::kUint32Value) {
                    LOG_INFO << "wrong value type for " << WIPER_FREQUENCY << std::endl;
                    continue;
                }

                auto wiper_target_position = actuator_targets.at(WIPER_TARGET_POSITION);
                if (wiper_target_position.value_case() != sdv::databroker::v1::Datapoint::ValueCase::kFloatValue) {
                    LOG_INFO << "wrong value type for " << WIPER_TARGET_POSITION << std::endl;
                    continue;
                }

                auto wiper_mode_value = wiper_mode.string_value();
                auto wiper_freq_value = wiper_freq.uint32_value();
                auto wiper_target_position_value = wiper_target_position.float_value();
                LOG_DEBUG << "wiper_mode_value: " << wiper_mode_value << std::endl;
                LOG_DEBUG << "wiper_freq_value: " << std::dec << (int)wiper_freq_value << std::endl;
                LOG_DEBUG << "wiper_target_position: " << wiper_target_position_value << std::endl;

                // TODO: mode -> e_WiperMode
                // serialize

                sdv::someip::wiper::e_WiperMode mode;
                if (!sdv::someip::wiper::wiper_mode_parse(wiper_mode_value, mode)) {
                    LOG_ERROR << "Invalid WiperMode value: " << wiper_mode_value << std::endl;
                    continue;
                }
                sdv::someip::wiper::t_WiperRequest req = { (uint8_t)wiper_freq_value, wiper_target_position_value, mode };
                uint8_t vss_payload[WIPER_SET_PAYLOAD_SIZE];
                if (!sdv::someip::wiper::serialize_vss_request(vss_payload, sizeof(vss_payload), req)) {
                    LOG_ERROR << "Failed to serialize WiperRequest: "
                            << sdv::someip::wiper::vss_request_to_string(req) << std::endl;
                    continue;
                }

                // Send SOME/IP request
                if (someip_client_) {
                    // someip_client_->SendRequest(wiper_mode_value, wiper_freq_value, wiper_target_position_value);
                    std::vector<vsomeip::byte_t> payload;
                    payload.insert(payload.end(), &vss_payload[0], &vss_payload[sizeof(vss_payload)]);
                    LOG_INFO << "Sending " << sdv::someip::wiper::vss_request_to_string(req) << std::endl;
                    someip_client_->SendRequest(
                            WIPER_VSS_SERVICE_ID,
                            WIPER_VSS_INSTANCE_ID,
                            WIPER_VSS_METHOD_ID,
                            payload);

                }
            } else {
                std::stringstream ss;
                for (auto kv : actuator_targets) {
                    ss << kv.first << " ";
                }
                LOG_INFO << "Not all actuator targets included in notification ["
                        << ss.str() << "]" << std::endl;
            }
        }
        actuator_target_subscriber_context_ = nullptr;
        LOG_INFO << "Disconnected" << std::endl;
    }
    LOG_INFO << "Exiting" << std::endl;
}

void SomeipFeederAdapter::Start() {
    LOG_INFO << "[SomeipFeederAdapter::" << __func__ << "] Starting adapter..." << std::endl;
    if (databroker_feeder_) {
        // start databropker feeder thread
        feeder_thread_ = std::make_shared<std::thread> (&sdv::broker_feeder::DataBrokerFeeder::Run, databroker_feeder_);
        int rc = pthread_setname_np(feeder_thread_->native_handle(), "broker_feeder");
        if (rc != 0) {
            LOG_ERROR << "Failed setting datafeeder thread name:" << rc << std::endl;
        }
    }
    if (someip_active_ && someip_client_) {
        // start vsomeip app "main" thread
        someip_thread_ = std::make_shared<std::thread> (&sdv::someip::SomeIPClient::Run, someip_client_);
        int rc = pthread_setname_np(someip_thread_->native_handle(), "someip_main");
        if (rc != 0) {
            LOG_ERROR << "Failed setting someip thread name:" << rc << std::endl;
        }
    }

    // start target actuator subscription
    actuator_target_subscriber_thread_ = std::make_shared<std::thread> ([this]() {
        RunActuatorTargetSubscriber();
    });
    int rc = pthread_setname_np(actuator_target_subscriber_thread_->native_handle(), "target_subscr");
    if (rc != 0) {
        LOG_ERROR << "Failed setting someip thread name:" << rc << std::endl;
    }

    feeder_active_ = true;
}

void SomeipFeederAdapter::Shutdown() {
    std::lock_guard<std::mutex> its_lock(shutdown_mutex_);
    LOG_DEBUG << "feeder_active_=" << feeder_active_
            << ", shutdown_requested_=" << shutdown_requested_<< std::endl;
    if (shutdown_requested_) {
        return;
    }
    shutdown_requested_ = true;
    feeder_active_ = false;

    if (actuator_target_subscriber_thread_) {
        actuator_target_subscriber_running_ = false;
        if (actuator_target_subscriber_context_) {
            actuator_target_subscriber_context_->TryCancel();
        }
    }

    if (feeder_thread_ && databroker_feeder_) {
        LOG_INFO << "Stopping databroker feeder..." << std::endl;
        databroker_feeder_->Shutdown();
    }
    if (someip_client_) {
        LOG_INFO << "Stopping someip client..." << std::endl;
        someip_client_->Shutdown();
        if (someip_thread_ && someip_thread_->joinable()) {
            if (someip_thread_->get_id() != std::this_thread::get_id()) {
                LOG_TRACE << "Joining someip thread..." << std::endl;
                someip_thread_->join();
                LOG_TRACE << "someip thread joined." << std::endl;
            } else {
                LOG_ERROR << "WARNING! Skipped joining someip from the same thread..." << std::endl;
                someip_thread_->detach();
            }
        }
    }
    // join feeder after stopping someip
    if (feeder_thread_ && feeder_thread_->joinable()) {
        LOG_TRACE << "Joining datafeeder thread..." << std::endl;
        feeder_thread_->join();
        LOG_TRACE << "datafeeder thread joined." << std::endl;
    }

    if (actuator_target_subscriber_thread_ && actuator_target_subscriber_thread_->joinable()) {
        actuator_target_subscriber_thread_->join();
    }
    LOG_TRACE << "done." << std::endl;
}

void SomeipFeederAdapter::FeedDummyData() {
    auto vss = WIPER_VSS_PATH + ".ActualPosition";
    auto vss_target = WIPER_VSS_PATH + ".ActualPosition";
    float target_pos = 110;

    if (!databroker_feeder_) {
        return;
    }
    LOG_INFO << "Starting dummy feeder" << std::endl;
    for (float pos=0.0; feeder_active_ && pos<target_pos; pos += 3.14) {
        { // feed ActualPosition
            LOG_INFO << "Feed Value " << pos << " to '" << vss << "'" << std::endl;
            sdv::databroker::v1::Datapoint datapoint;
            datapoint.set_float_value(pos);
            if (!datapoint.has_timestamp()) {
                auto ts = datapoint.mutable_timestamp();
                struct timespec tv;
                int rc = clock_gettime(CLOCK_REALTIME, &tv);
                if (rc == 0) {
                    ts->set_seconds(tv.tv_sec);
                    ts->set_nanos(tv.tv_nsec);
                }
            }
            databroker_feeder_->FeedValue(vss, datapoint);
        }
        { // feed TargetPosition
            LOG_INFO << "Feed Value " << target_pos << " to '" << vss_target << "'" << std::endl;
            sdv::databroker::v1::Datapoint datapoint;
            datapoint.set_float_value(target_pos);
            databroker_feeder_->FeedValue(vss_target, datapoint);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

int SomeipFeederAdapter::on_someip_message(
            vsomeip::service_t service_id, vsomeip::instance_t instance_id, vsomeip::method_t method_id,
            const uint8_t *payload, size_t payload_length)
{
    if (service_id == WIPER_VSS_SERVICE_ID &&
        instance_id == WIPER_VSS_INSTANCE_ID &&
        method_id == WIPER_VSS_METHOD_ID)
    {
        LOG_INFO << "Received Response from ["
            << std::setw(4) << std::setfill('0') << std::hex
            << service_id << "."
            << std::setw(4) << std::setfill('0') << std::hex
            << instance_id << "."
            << std::setw(4) << std::setfill('0') << std::hex
            << method_id << "], payload ["
            << sdv::someip::hexdump((uint8_t*)payload, payload_length) << "]"
            << std::endl;

        return 0;
    }
    if (service_id  != WIPER_SERVICE_ID ||
        instance_id != WIPER_INSTANCE_ID ||
        method_id   != WIPER_EVENT_ID)
    {

        LOG_ERROR << "Ignored non-wiper event ["
            << std::setw(4) << std::setfill('0') << std::hex
            << service_id << "."
            << std::setw(4) << std::setfill('0') << std::hex
            << instance_id << "."
            << std::setw(4) << std::setfill('0') << std::hex
            << method_id << "]" << std::endl;
            return -1;
    }

    sdv::someip::wiper::t_Event event;
    if (sdv::someip::wiper::deserialize_event(payload, payload_length, event)) {
        if (someip_client_->GetConfig().debug > 0) {
            LOG_DEBUG << "Received "
                    << sdv::someip::wiper::event_to_string(event) << std::endl;
        }
        // sdv::someip::wiper::print_status(std::string("### [SomeipFeederAdapter::") + __func__ + "] ", event);
		sdv::someip::wiper::print_status(std::string("### "), event);

        // feed values to kuksa databroker
        sdv::broker_feeder::DatapointValues values = {
            { WIPER_ACTUAL_POSITION,
                sdv::broker_feeder::createDatapoint(event.data.ActualPosition) },
            { WIPER_DRIVE_CURRENT,
                sdv::broker_feeder::createDatapoint(event.data.DriveCurrent) },
            { WIPER_IS_WIPING,
                sdv::broker_feeder::createDatapoint((bool)event.data.isWiping) },
            { WIPER_IS_BLOCKED,
                sdv::broker_feeder::createDatapoint((bool)event.data.isBlocked) },
            { WIPER_IS_ENDING_WIPE_CYCLE,
                sdv::broker_feeder::createDatapoint((bool)event.data.isEndingWipeCycle) },
            { WIPER_IS_OVERHEATED,
                sdv::broker_feeder::createDatapoint((bool)event.data.isOverheated) },
            { WIPER_IS_POSITION_REACHED,
                sdv::broker_feeder::createDatapoint((bool)event.data.isPositionReached) },
            { WIPER_IS_WIPER_ERROR,
                sdv::broker_feeder::createDatapoint((bool)event.data.isWiperError) }
        };
        databroker_feeder_->FeedValues(values);
        return 0;
    }
    LOG_ERROR << "Deserializaton failed!" << std::endl;
    return -2; // deserialize failed
}


}  // namespace adapter
}  // namespace sdv
