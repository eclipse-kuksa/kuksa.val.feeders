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
#include <chrono>
#include <random>
#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <cstring>

#include <vsomeip/vsomeip.hpp>

#include "wiper_poc.h"


static int debug = ::getenv("DEBUG") ? ::atoi(::getenv("DEBUG")) : 0;

class wiper_service {
public:
    wiper_service(bool _use_tcp, uint32_t _cycle) :
            app_(vsomeip::runtime::get()->create_application()),
            is_registered_(false),
            use_tcp_(_use_tcp),
            cycle_(_cycle),
            blocked_(false),
            running_(true),
            is_offered_(false),
            offer_thread_(std::bind(&wiper_service::run, this)),
            notify_thread_(std::bind(&wiper_service::notify, this)) {
    }

    bool init() {
        std::lock_guard<std::mutex> its_lock(mutex_);

        if (!app_->init()) {
            std::cerr << "Couldn't initialize application" << std::endl;
            return false;
        }
        app_->register_state_handler(
                std::bind(&wiper_service::on_state, this,
                        std::placeholders::_1));

        std::set<vsomeip::eventgroup_t> its_groups;
        its_groups.insert(WIPER_EVENTGROUP_ID);
        app_->offer_event(
                WIPER_SERVICE_ID,
                WIPER_INSTANCE_ID,
                WIPER_EVENT_ID,
                its_groups,
                vsomeip::event_type_e::ET_FIELD, std::chrono::milliseconds::zero(),
                false, true, nullptr,
                use_tcp_ ? vsomeip::reliability_type_e::RT_RELIABLE : vsomeip::reliability_type_e::RT_UNRELIABLE
                // vsomeip::reliability_type_e::RT_UNKNOWN
            );
        {
            std::lock_guard<std::mutex> its_lock(payload_mutex_);
            payload_ = vsomeip::runtime::get()->create_payload();
        }

        blocked_ = true;
        condition_.notify_one();
        return true;
    }

    void start() {
        app_->start();
    }

    /*
     * Handle signal to shutdown
     */
    void stop() {
        running_ = false;
        blocked_ = true;
        condition_.notify_one();
        notify_condition_.notify_one();
        app_->clear_all_handler();
        stop_offer();
        offer_thread_.join();
        notify_thread_.join();
        app_->stop();
    }

    void offer() {
        std::lock_guard<std::mutex> its_lock(notify_mutex_);
        std::printf("Application %s offering [%04x.%04x] v%u.%u\n",
                app_->get_name().c_str(),
                WIPER_SERVICE_ID, WIPER_INSTANCE_ID,
                WIPER_SERVICE_MAJOR, WIPER_SERVICE_MINOR);

        app_->offer_service(WIPER_SERVICE_ID, WIPER_INSTANCE_ID,
                WIPER_SERVICE_MAJOR, WIPER_SERVICE_MINOR);
        is_offered_ = true;
        notify_condition_.notify_one();
    }

    void stop_offer() {
        std::printf("Application %s stop offering [%04x.%04x] v%u.%u\n",
                app_->get_name().c_str(), WIPER_SERVICE_ID, WIPER_INSTANCE_ID,
                WIPER_SERVICE_MAJOR, WIPER_SERVICE_MINOR);
        app_->stop_offer_service(WIPER_SERVICE_ID, WIPER_INSTANCE_ID,
                WIPER_SERVICE_MAJOR, WIPER_SERVICE_MINOR);
        is_offered_ = false;
    }

    void on_state(vsomeip::state_type_e _state) {
        std::cout << "Application " << app_->get_name() << " is "
        << (_state == vsomeip::state_type_e::ST_REGISTERED ?
                "registered." : "deregistered.") << std::endl;

        if (_state == vsomeip::state_type_e::ST_REGISTERED) {
            if (!is_registered_) {
                is_registered_ = true;
            }
        } else {
            is_registered_ = false;
        }
    }

    void run() {
        std::unique_lock<std::mutex> its_lock(mutex_);
        while (!blocked_)
            condition_.wait(its_lock);

        bool is_offer(true);
        while (running_) {
            if (is_offer)
                offer();
            else
                stop_offer();

            for (int i = 0; i < 10 && running_; i++)
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            // is_offer = !is_offer; // Disabled toggling of event availability each 10sec
        }
    }

    void notify() {
        std::shared_ptr<vsomeip::message> its_message
            = vsomeip::runtime::get()->create_request(use_tcp_);

        its_message->set_service(WIPER_SERVICE_ID);
        its_message->set_instance(WIPER_INSTANCE_ID);
        its_message->set_method(WIPER_EVENT_ID);

        sdv::someip::wiper::t_Event event;
        uint32_t its_size = sizeof(event);

        event.data.ActualPosition = 90;
        event.data.DriveCurrent = 10;
        event.data.TempGear = 100;
        event.data.isBlocked = false;
        event.data.isEndingWipeCycle = false;
        event.data.isOverheated = false;
        event.data.isPositionReached = false;
        event.data.isWiperError = false;
        event.data.isWiping = true;
        event.data.ECUTemp = 75;
        event.data.LINError = 255;
        event.data.isUnderVoltage = false;
        event.data.isOverVoltage = false;

        uint8_t its_data[its_size];

        // model init
        event.sequenceCounter = 0;

        const float default_pos_step = 3.13f;
        const float default_current = 10.0f;
        float pos_step = default_pos_step;

        std::random_device rd;  // Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
        std::uniform_real_distribution<float> current_rnd(-0.05f, 0.05f);
        std::uniform_real_distribution<float> speed_rnd(0.0f, default_pos_step/3.0f);

        uint32_t counter = 0;
        while (running_)
        {
            std::unique_lock<std::mutex> its_lock(notify_mutex_);
            while (!is_offered_ && running_)
                notify_condition_.wait(its_lock);
            while (is_offered_ && running_)
            {
                // run model step
                counter++;
                event.sequenceCounter = (uint8_t)(counter & 0xFF);

                // toggle isOverheated ~9s
                if ((cycle_ * counter) % 9000 == 0) {
                    event.data.isOverheated = !event.data.isOverheated;
                    std::cout << std::endl << "*** wiper "
                        << (event.data.isOverheated ? "Overheated." : "not Overheated") << std::endl << std::endl;
                }

                // toggle isWiping ~5s
                if ((cycle_ * counter) % 5000 == 0) {
                    event.data.isWiping = !event.data.isWiping;
                    std::cout << std::endl << "*** wiping "
                        << (event.data.isWiping ? "started." : "stopped.") << std::endl << std::endl;
                }

                // simulate wiper movement
                if (event.data.isWiping) {
                    if (event.data.ActualPosition >= 150.0f) {
                        pos_step = -default_pos_step;
                    } else
                    if (event.data.ActualPosition < default_pos_step) {
                        pos_step = default_pos_step;
                    }
                    event.data.ActualPosition += pos_step + speed_rnd(gen);

                    event.data.DriveCurrent = default_current + current_rnd(gen);
                } else {
                    event.data.DriveCurrent = 0.0f;
                }

                std::printf("[EVENT] Seq:%3d, ActualPos: %f, DriveCurrent: %f\n",
                    (int)event.sequenceCounter, event.data.ActualPosition, event.data.DriveCurrent);

                // serialize t_Event as someip payload
                its_data[0] = event.sequenceCounter;
                sdv::someip::wiper::float_to_bytes(event.data.ActualPosition, &its_data[1]);
                sdv::someip::wiper::float_to_bytes(event.data.DriveCurrent, &its_data[5]);
                its_data[9]  = event.data.TempGear;
                its_data[10] = event.data.isWiping;
                its_data[11] = event.data.isEndingWipeCycle;
                its_data[12] = event.data.isWiperError;
                its_data[13] = event.data.isPositionReached;
                its_data[14] = event.data.isBlocked;
                its_data[15] = event.data.isOverheated;
                its_data[16] = event.data.ECUTemp;
                its_data[17] = event.data.LINError;
                its_data[18] = event.data.isOverVoltage;
                its_data[19] = event.data.isUnderVoltage;

                {
                    std::lock_guard<std::mutex> its_lock(payload_mutex_);
                    payload_->set_data(its_data, its_size);
                    if (debug > 0) {
                        std::printf("### app.notify(%04x.%04x/%04x) -> %u bytes\n",
                                WIPER_SERVICE_ID, WIPER_INSTANCE_ID, WIPER_EVENT_ID, its_size);
                    }
                    if (debug > 1) {
                        std::cout << "### Notify payload: "
                                << sdv::someip::wiper::bytes_to_string(its_data, its_size)
                                << "]" << std::endl;
                    }
                    app_->notify(WIPER_SERVICE_ID, WIPER_INSTANCE_ID, WIPER_EVENT_ID, payload_);
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(cycle_));
            }
        }
    }

private:
    std::shared_ptr<vsomeip::application> app_;
    bool is_registered_;
    bool use_tcp_;
    uint32_t cycle_;

    std::mutex mutex_;
    std::condition_variable condition_;
    bool blocked_;
    bool running_;

    std::mutex notify_mutex_;
    std::condition_variable notify_condition_;
    bool is_offered_;

    std::mutex payload_mutex_;
    std::shared_ptr<vsomeip::payload> payload_;

    // blocked_ / is_offered_ must be initialized before starting the threads!
    std::thread offer_thread_;
    std::thread notify_thread_;
};

// #ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
wiper_service *its_sample_ptr(nullptr);

void handle_signal(int _signal) {
    if (its_sample_ptr != nullptr &&
            (_signal == SIGINT || _signal == SIGTERM))
        its_sample_ptr->stop();
}
// #endif

int main(int argc, char **argv) {
    bool use_tcp = false;
    uint32_t cycle = 1000; // default 1s

    std::string tcp_enable("--tcp");
    std::string udp_enable("--udp");
    std::string cycle_arg("--cycle");

    // sanity checks for VSOMEIP environment
    const char* app_name = ::getenv("VSOMEIP_APPLICATION_NAME");
    if (!app_name) {
        std::cerr << "Environment variable VSOMEIP_APPLICATION_NAME not set!" << std::endl;
        return 1;
    }

    const char* app_config = ::getenv("VSOMEIP_CONFIGURATION");
    if (!app_config) {
        std::cerr << "Environment variable VSOMEIP_CONFIGURATION not set!" << std::endl;
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (tcp_enable == argv[i]) {
            use_tcp = true;
            break;
        }
        if (udp_enable == argv[i]) {
            use_tcp = false;
            break;
        }

        if (cycle_arg == argv[i] && i + 1 < argc) {
            i++;
            std::stringstream converter;
            converter << argv[i];
            converter >> cycle;
        }
    }

    wiper_service its_sample(use_tcp, cycle);
// #ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
    its_sample_ptr = &its_sample;
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
// #endif
    if (its_sample.init()) {
        its_sample.start();
        return 0;
    } else {
        return 1;
    }
}