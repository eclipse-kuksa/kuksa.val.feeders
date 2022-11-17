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

#ifndef SOMEIP_KUKSA_ADAPTER_H
#define SOMEIP_KUKSA_ADAPTER_H

#include <string>
#include <condition_variable>
#include <mutex>


#include "someip_client.h"
#include "data_broker_feeder.h"

namespace sdv {
namespace adapter {

/**
 * @brief Base path for Wiper data in VSS 3.0
 */
const std::string WIPER_VSS_PATH = "Vehicle.Body.Windshield.Front.Wiping.System";

class SomeipFeederAdapter {

  public:
    SomeipFeederAdapter();
    virtual ~SomeipFeederAdapter();

    bool InitDataBrokerFeeder(const std::string &databroker_addr);
    bool InitSomeipClient(sdv::someip::SomeIPConfig _config);

    /**
     * @brief Starts someip and databroker feeder threads
     */
    void Start();

    /**
     * @brief Terminates someip and databroker feeder threads
     */
    void Shutdown();

    /**
     * @brief Sends dummy data to databroker feeder (someip not used)
     */
    void FeedDummyData();

  protected:
    int on_someip_message(
            vsomeip::service_t service,
            vsomeip::instance_t instance,
            vsomeip::method_t event,
            const uint8_t *payload,
            size_t payload_length);

private:
    void RunActuatorTargetSubscriber();

    std::atomic<bool> feeder_active_;

    std::string databroker_addr_;
    std::shared_ptr<sdv::broker_feeder::CollectorClient> collector_client_;

    // Feeder
    std::shared_ptr<sdv::broker_feeder::DataBrokerFeeder> databroker_feeder_;
    std::shared_ptr<std::thread> feeder_thread_;

    // Actuator target subscriber
    std::shared_ptr<std::thread> actuator_target_subscriber_thread_;
    std::atomic<bool> actuator_target_subscriber_running_;
    std::unique_ptr<grpc::ClientContext> actuator_target_subscriber_context_;

    // SOME/IP client
    bool someip_use_tcp_;
    std::shared_ptr<sdv::someip::SomeIPClient> someip_client_;
    std::shared_ptr<std::thread> someip_thread_;
    std::atomic<bool> someip_active_;

    std::mutex shutdown_mutex_;
    std::atomic<bool> shutdown_requested_;
    int log_level_;
};

}  // namespace adapter
}  // namespace sdv

#endif // SOMEIP_KUKSA_ADAPTER_H
