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
#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#include <vsomeip/vsomeip.hpp>

#include "wiper_poc.h"

static int debug = ::getenv("DEBUG") ? ::atoi(::getenv("DEBUG")) : 1;

class wiler_client {
public:
    wiler_client(bool _use_tcp) :
            app_(vsomeip::runtime::get()->create_application()), use_tcp_(
                    _use_tcp) {
    }

    bool init() {
        if (!app_->init()) {
            std::cerr << "Couldn't initialize application" << std::endl;
            return false;
        }
        std::cout << "Client settings [protocol="
                << (use_tcp_ ? "TCP" : "UDP")
                << "]"
                << std::endl;

        app_->register_state_handler(
                std::bind(&wiler_client::on_state, this,
                        std::placeholders::_1));

        app_->register_message_handler(
                vsomeip::ANY_SERVICE, vsomeip::ANY_INSTANCE, vsomeip::ANY_METHOD,
//                WIPER_SERVICE_ID, WIPER_INSTANCE_ID, WIPER_METHOD_ID,
                std::bind(&wiler_client::on_message, this,
                        std::placeholders::_1));

        app_->register_availability_handler(
                WIPER_SERVICE_ID, WIPER_INSTANCE_ID,
                std::bind(&wiler_client::on_availability,
                          this,
                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                WIPER_SERVICE_MAJOR, WIPER_SERVICE_MINOR);
#if 0
        std::set<vsomeip::eventgroup_t> its_groups;
        its_groups.insert(WIPER_EVENTGROUP_ID);
        app_->request_event(
                WIPER_SERVICE_ID, WIPER_INSTANCE_ID, WIPER_EVENT_ID,
                its_groups, vsomeip::event_type_e::ET_FIELD,
                (use_tcp_ ? vsomeip::reliability_type_e::RT_RELIABLE : vsomeip::reliability_type_e::RT_UNRELIABLE));

        app_->subscribe(WIPER_SERVICE_ID, WIPER_INSTANCE_ID, WIPER_EVENTGROUP_ID, WIPER_SERVICE_MAJOR);
#endif

        return true;
    }

    void start() {
        app_->start();
    }

    /*
     * Handle signal to shutdown
     */
    void stop() {
        app_->clear_all_handler();
        app_->unsubscribe(WIPER_SERVICE_ID, WIPER_INSTANCE_ID, WIPER_EVENTGROUP_ID);
        app_->release_event(WIPER_SERVICE_ID, WIPER_INSTANCE_ID, WIPER_EVENT_ID);
        app_->release_service(WIPER_SERVICE_ID, WIPER_INSTANCE_ID);
        app_->stop();
    }

    void on_state(vsomeip::state_type_e _state) {
        if (_state == vsomeip::state_type_e::ST_REGISTERED) {
            app_->request_service(WIPER_SERVICE_ID, WIPER_INSTANCE_ID, WIPER_SERVICE_MAJOR, WIPER_SERVICE_MINOR);
        }
    }

    void on_availability(vsomeip::service_t _service, vsomeip::instance_t _instance, bool _is_available) {
        std::cout << "Service ["
                << std::setw(4) << std::setfill('0') << std::hex << _service << "." << _instance
                << "] is "
                << (_is_available ? "available." : "NOT available.")
                << std::endl;
		if (_is_available) {

			std::set<vsomeip::eventgroup_t> its_groups;
			its_groups.insert(WIPER_EVENTGROUP_ID);
			app_->request_event(
					WIPER_SERVICE_ID, WIPER_INSTANCE_ID, WIPER_EVENT_ID,
					its_groups, vsomeip::event_type_e::ET_FIELD,
					(use_tcp_ ? vsomeip::reliability_type_e::RT_RELIABLE : vsomeip::reliability_type_e::RT_UNRELIABLE));

			app_->subscribe(WIPER_SERVICE_ID, WIPER_INSTANCE_ID, WIPER_EVENTGROUP_ID, WIPER_SERVICE_MAJOR);
		}
    }

    void on_message(const std::shared_ptr<vsomeip::message> &_response) {
        std::stringstream its_message;
        sdv::someip::wiper::t_Event event;

        its_message << "Received a notification for Event ["
                << std::setw(4)    << std::setfill('0') << std::hex
                << _response->get_service() << "."
                << std::setw(4) << std::setfill('0') << std::hex
                << _response->get_instance() << "."
                << std::setw(4) << std::setfill('0') << std::hex
                << _response->get_method() << "] to Client/Session ["
                << std::setw(4) << std::setfill('0') << std::hex
                << _response->get_client() << "/"
                << std::setw(4) << std::setfill('0') << std::hex
                << _response->get_session()
                << "] = ";
        std::shared_ptr<vsomeip::payload> its_payload =
                _response->get_payload();
        its_message << "(" << std::dec << its_payload->get_length() << ") ";

        if (debug > 0) {
            its_message << sdv::someip::wiper::bytes_to_string(its_payload->get_data(), its_payload->get_length());
        }
        // for (uint32_t i = 0; i < its_payload->get_length(); ++i)
        //     its_message << std::hex << std::setw(2) << std::setfill('0')
        //         << (int) its_payload->get_data()[i] << " ";
        std::cout << its_message.str() << std::endl;
        sdv::someip::wiper::deserialize_event(
                reinterpret_cast<const uint8_t*>(its_payload->get_data()),
                its_payload->get_length(),
                event);
        sdv::someip::wiper::print_status("###", event);
    }

private:
    std::shared_ptr< vsomeip::application > app_;
    bool use_tcp_;
};


wiler_client *its_sample_ptr(nullptr);
void handle_signal(int _signal) {
    if (its_sample_ptr != nullptr &&
            (_signal == SIGINT || _signal == SIGTERM))
        its_sample_ptr->stop();
}


int main(int argc, char **argv) {
    bool use_tcp = false;

    std::string tcp_enable("--tcp");
    std::string udp_enable("--udp");

    int i = 1;
    while (i < argc) {
        if (tcp_enable == argv[i]) {
            use_tcp = true;
        } else if (udp_enable == argv[i]) {
            use_tcp = false;
        }
        i++;
    }

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

    wiler_client its_sample(use_tcp);

    its_sample_ptr = &its_sample;
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    if (its_sample.init()) {
        its_sample.start();
        return 0;
    } else {
        return 1;
    }
}
