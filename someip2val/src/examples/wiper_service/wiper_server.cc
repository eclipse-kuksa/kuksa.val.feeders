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
#include "wiper_sim.h"


int debug = ::getenv("DEBUG") ? ::atoi(::getenv("DEBUG")) : 0;

static int sim_auto = ::getenv("SIM_AUTO") ? ::atoi(::getenv("SIM_AUTO")) : 0;

using namespace sdv::someip::wiper;

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
            wiper_sim_(cycle_),
            offer_thread_(std::bind(&wiper_service::run, this)),
            notify_thread_(std::bind(&wiper_service::notify_th, this))
    {
        pthread_setname_np(offer_thread_.native_handle(), "wiper_run");
        pthread_setname_np(notify_thread_.native_handle(), "wiper_notify");
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
        ////
        // register wiper event service
        //
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

        //
        // register wiper vss service
        //

        // register a callback for responses from the service
        app_->register_message_handler(
                WIPER_VSS_SERVICE_ID,
                WIPER_VSS_INSTANCE_ID,
                WIPER_VSS_METHOD_ID,
                std::bind(&wiper_service::on_vss_message_cb, this,
                        std::placeholders::_1));

        // register a callback which is called as soon as the service is available
        app_->register_availability_handler(
                WIPER_VSS_SERVICE_ID,
                WIPER_VSS_INSTANCE_ID,
                std::bind(&wiper_service::on_availability_cb, this,
                        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        blocked_ = true;
        condition_.notify_one();
        return true;
    }

    void start() {
        app_->start();
    }

    void on_vss_message_cb(const std::shared_ptr<vsomeip::message> &_request) {
        std::stringstream its_message;
        t_Event event;

        its_message << "### [VSS] Received a Request for ["
                << std::setw(4)    << std::setfill('0') << std::hex
                << _request->get_service() << "."
                << std::setw(4) << std::setfill('0') << std::hex
                << _request->get_instance() << "."
                << std::setw(4) << std::setfill('0') << std::hex
                << _request->get_method() << "] to Client/Session ["
                << std::setw(4) << std::setfill('0') << std::hex
                << _request->get_client() << "/"
                << std::setw(4) << std::setfill('0') << std::hex
                << _request->get_session()
                << "] = ";
        std::shared_ptr<vsomeip::payload> its_payload = _request->get_payload();
        its_message << "(" << std::dec << its_payload->get_length() << ")";
        if (debug > 1) {
            its_message << " [";
            for (int i=0; i<its_payload->get_length(); i++) {
                its_message
                        << std::setw(2) << std::setfill('0') << std::hex
                        << (int)its_payload->get_data()[i] << " ";
            }
            its_message << "]";
        }

        std::cout << std::endl;
        std::cout << its_message.str() << std::endl;
        std::cout << std::endl;

        // TODO create and send response
        std::shared_ptr<vsomeip::message> its_response = vsomeip::runtime::get()->create_response(_request);
        std::shared_ptr<vsomeip::payload> resp_payload = vsomeip::runtime::get()->create_payload();

        uint8_t result = 0x01; // or 0x01 for error

        t_WiperRequest wiper_request;
        if (deserialize_vss_request(its_payload->get_data(), its_payload->get_length(), wiper_request)) {
            std::cout << "### [VSS] received: "
                    << vss_request_to_string(wiper_request)
                    << std::endl;
            // TODO: start/stop wiping, etc.
            wiper_sim_.model_set(wiper_request);
            result = 0x00; // ok
        } else {
            std::cerr << "### [VSS] Failed to deserialize payload!" << std::endl;
        }

        std::vector<vsomeip::byte_t> its_payload_data;
        its_payload_data.push_back(result);

        resp_payload->set_data(its_payload_data);
        its_response->set_payload(resp_payload);

        if (debug > 0) std::cout << "### [VSS] Sending VSS Response..." << std::endl;
        app_->send(its_response);
        if (debug > 1) std::cout << "### [VSS] done." << std::endl;
    }

    void on_availability_cb(vsomeip::service_t _service, vsomeip::instance_t _instance, bool _is_available) {
        std::cout << "### [VSS] Service ["
                << std::setw(4) << std::setfill('0') << std::hex << _service
                << "."
                << std::setw(4) << std::setfill('0') << std::hex << _instance
                << "] is "
                << (_is_available ? "available." : "NOT available.")
                << std::endl;
		if (_is_available) {
            // TODO: what?
		}
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
        if (offer_thread_.joinable()) {
            offer_thread_.join();
        }
        if (notify_thread_.joinable()) {
            notify_thread_.join();
        }
        app_->stop();
    }

    void offer() {
        std::lock_guard<std::mutex> its_lock(notify_mutex_);
        std::printf("Application %s offering VSS [%04x.%04x] v%u.%u\n",
                app_->get_name().c_str(),
                WIPER_VSS_SERVICE_ID, WIPER_VSS_INSTANCE_ID,
                WIPER_VSS_SERVICE_MAJOR, WIPER_VSS_SERVICE_MINOR);

        app_->offer_service(WIPER_VSS_SERVICE_ID, WIPER_VSS_INSTANCE_ID,
                WIPER_VSS_SERVICE_MAJOR, WIPER_VSS_SERVICE_MINOR);

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

        std::printf("Application %s offering VSS [%04x.%04x] v%u.%u\n",
                app_->get_name().c_str(),
                WIPER_VSS_SERVICE_ID, WIPER_VSS_INSTANCE_ID,
                WIPER_VSS_SERVICE_MAJOR, WIPER_VSS_SERVICE_MINOR);

        app_->stop_offer_service(WIPER_VSS_SERVICE_ID, WIPER_VSS_INSTANCE_ID,
                WIPER_VSS_SERVICE_MAJOR, WIPER_VSS_SERVICE_MINOR);

        is_offered_ = false;
    }

    void on_state(vsomeip::state_type_e _state) {
        std::cout << "Application " << app_->get_name() << " is "
                << (_state == vsomeip::state_type_e::ST_REGISTERED ?
                "registered." : "deregistered.") << std::endl;

        is_registered_ = (_state == vsomeip::state_type_e::ST_REGISTERED);
        if (is_registered_) {
            // we are registered at the runtime and can offer our service
            // offer(); -> this generates a blocking state handler!
        }
    }

    void run() {
        // offer thread, reused for set as well
        std::unique_lock<std::mutex> its_lock(mutex_);
        while (!blocked_)
            condition_.wait(its_lock);

        bool is_offer(true);

        // predefined vss positions
        int vss_index = 0;
        t_WiperRequest vss_reqests[] = {
            { 40, 80.0f, e_WiperMode::WIPE },
            { 50, 20.0f, e_WiperMode::WIPE },
            { 30, 10.0f, e_WiperMode::WIPE },
            { 30,  2.0f, e_WiperMode::WIPE },
            { 70,  2.0f, e_WiperMode::EMERGENCY_STOP }
        };

        while (running_) {
            if (is_offer)
                offer();
            else
                stop_offer();

            if (sim_auto) {
                std::cout << "# SIM: Setting VSS [" << vss_index << "] ..." << std::endl;
                wiper_sim_.model_set(vss_reqests[vss_index++]);
                if (vss_index > 5) {
                    vss_index = 0;
                }
            }
            for (int i = 0; i < 10 && running_; i++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }

            //is_offer = !is_offer; // Disabled toggling of event availability each 10sec
        }
    }

    void notify_th() {
        std::shared_ptr<vsomeip::message> its_message
            = vsomeip::runtime::get()->create_request(use_tcp_);

        its_message->set_service(WIPER_SERVICE_ID);
        its_message->set_instance(WIPER_INSTANCE_ID);
        its_message->set_method(WIPER_EVENT_ID);
        // its_message->set_interface_version(WIPER_SERVICE_MAJOR);

        t_Event event;
        uint8_t its_data[WIPER_EVENT_PAYLOAD_SIZE];
        size_t its_size = sizeof(its_data);

        while (running_)
        {
            std::unique_lock<std::mutex> its_lock(notify_mutex_);
            while (!is_offered_ && running_)
                notify_condition_.wait(its_lock);

            //auto now = std::chrono::system_clock::now();
            auto event_ts = 0;//std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
            auto sim_step = 500;
            if (sim_step > cycle_) {
                sim_step = cycle_;
            }
            while (is_offered_ && running_)
            {
                // run model step
                bool was_cycle_ending = wiper_sim_.is_cycle_ending();
                wiper_sim_.model_step(event);
                std::this_thread::sleep_for(std::chrono::milliseconds(sim_step)); // run cycle 10 ms
                event_ts += sim_step; // FIXME: use ts delta
                if (!is_offered_ || !running_) {
                    break;
                }
                if (event_ts >= cycle_ || was_cycle_ending != wiper_sim_.is_cycle_ending()) {
                    if (debug > 1) {
                        std::printf("[EVENT] ActualPos:%6.2f, DriveCurrent:%5.2f, Wiping:%d, CycEnd:%d, PosReach:%d, Seq:%3d, [%5.3f]\n",
                                event.data.ActualPosition, event.data.DriveCurrent,
                                event.data.isWiping, event.data.isEndingWipeCycle, event.data.isPositionReached,
                                (int)event.sequenceCounter, (float)event_ts / 1000.0f);
                    }

                    event_ts = 0;
                    if (serialize_wiper_event(event, (uint8_t*)&its_data, its_size)) {
                        std::lock_guard<std::mutex> its_lock(payload_mutex_);
                        payload_->set_data(its_data, its_size);
                        if (debug > 2) {
                            std::printf("### app.notify(%04x.%04x/%04x) -> %lu bytes\n",
                                    WIPER_SERVICE_ID, WIPER_INSTANCE_ID, WIPER_EVENT_ID, its_size);
                        }
                        if (debug > 3) {
                            std::cout << "### Notify payload: "
                                    << bytes_to_string(its_data, its_size)
                                    << "]" << std::endl;
                        }
                        app_->notify(WIPER_SERVICE_ID, WIPER_INSTANCE_ID, WIPER_EVENT_ID, payload_);
                    }
                }
            }
        }
    }

#if 0
    void notify_th_random() {
        std::shared_ptr<vsomeip::message> its_message
            = vsomeip::runtime::get()->create_request(use_tcp_);

        its_message->set_service(WIPER_SERVICE_ID);
        its_message->set_instance(WIPER_INSTANCE_ID);
        its_message->set_method(WIPER_EVENT_ID);
        // its_message->set_interface_version(WIPER_SERVICE_MAJOR);

        t_Event event;
        uint32_t its_size = sizeof(event);
        uint8_t its_data[its_size];

        // wiper_model_init(event);

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

                if (debug > 0) {
                    std::printf("[EVENT] Seq:%3d, ActualPos: %f, DriveCurrent: %f\n",
                            (int)event.sequenceCounter, event.data.ActualPosition, event.data.DriveCurrent);
                }
                // serialize t_Event as someip payload
                its_data[0] = event.sequenceCounter;
                float_to_bytes(event.data.ActualPosition, &its_data[1]);
                float_to_bytes(event.data.DriveCurrent, &its_data[5]);
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
                    if (debug > 1) {
                        std::printf("### app.notify(%04x.%04x/%04x) -> %u bytes\n",
                                WIPER_SERVICE_ID, WIPER_INSTANCE_ID, WIPER_EVENT_ID, its_size);
                    }
                    if (debug > 2) {
                        std::cout << "### Notify payload: "
                                << bytes_to_string(its_data, its_size)
                                << "]" << std::endl;
                    }
                    app_->notify(WIPER_SERVICE_ID, WIPER_INSTANCE_ID, WIPER_EVENT_ID, payload_);
                }
                if (running_) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(cycle_));
                }
            }
        }
    }
#endif

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

    // model simulator
    t_Event model_event;
    wiper_simulator wiper_sim_;
};

wiper_service *its_sample_ptr(nullptr);

void handle_signal(int _signal) {
    if (its_sample_ptr != nullptr &&
            (_signal == SIGINT || _signal == SIGTERM))
        its_sample_ptr->stop();
}
// #endif

int main(int argc, char **argv) {
    bool use_tcp = false;
    uint32_t cycle = 100; // default 1s

    std::string tcp_enable("--tcp");
    std::string udp_enable("--udp");
    std::string cycle_arg("--cycle");
    std::string sim_arg("--sim");

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

    if (vsomeip::DEFAULT_MAJOR != 0) {
        std::cout << "# Warning: compiled with vsomeip::DEFAULT_MAJOR=" << std::dec << (int)vsomeip::DEFAULT_MAJOR << std::endl;
    }

    const char* cycle_env= ::getenv("CYCLE");
    if (cycle_env) {
        cycle = atoi(cycle_env);
    }
    for (int i = 1; i < argc; i++) {
        if (tcp_enable == argv[i]) {
            use_tcp = true;
            break;
        } else
        if (udp_enable == argv[i]) {
            use_tcp = false;
            break;
        }
        else
        if (cycle_arg == argv[i] && i + 1 < argc) {
            i++;
            std::stringstream converter;
            converter << argv[i];
            converter >> cycle;
        } else
        if (sim_arg == argv[i]) {
            sim_auto = 1; // override envrionment
            break;
        }

    }

    wiper_service its_sample(use_tcp, cycle);
    its_sample_ptr = &its_sample;
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    if (its_sample.init()) {
        its_sample.start();
        return 0;
    } else {
        its_sample.stop();
        return 1;
    }
}
