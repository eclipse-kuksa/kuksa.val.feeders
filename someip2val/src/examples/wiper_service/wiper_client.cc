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

using namespace sdv::someip::wiper;

// fw decl
std::string message_type_to_string(vsomeip::message_type_e msg_type);

// struct request_status_t {
//     vsomeip::return_code_e code;
//     std::shared_ptr<vsomeip::payload> payload;
// };

class wiper_client {

private:
    std::shared_ptr<vsomeip::application> app_;
    bool use_tcp_;
    bool use_vss_;
    bool use_events_;
    bool is_registered_;
    t_WiperRequest vss_req_; // vss request to send once

    bool blocked_;
    bool running_;

    std::mutex mutex_;
    std::condition_variable condition_;

    std::mutex request_mutex_;
    std::condition_variable request_condition_;
    // request_status_t request_status_;

    // blocked_ / is_offered_ must be initialized before starting the threads!
    // std::thread request_thread_;

public:
    wiper_client(bool _use_tcp, bool _use_events=true, bool _use_vss=false, t_WiperRequest _vss={}) :
        app_(vsomeip::runtime::get()->create_application())
        , use_tcp_(_use_tcp)
        , use_vss_(_use_vss)
        , vss_req_(_vss)
        , use_events_(_use_events)
        , is_registered_(false)
        , blocked_(false)
        , running_(true)
        //, request_thread_(std::bind(&wiper_client::run, this))
    {
        // pthread_setname_np(request_thread_.native_handle(), "request_thread");
    }

    bool init() {
        std::lock_guard<std::mutex> its_lock(mutex_);
        if (!app_->init()) {
            std::cerr << "Couldn't initialize application" << std::endl;
            return false;
        }
        std::printf("### Client settings [cli_id=0x%04x, app='%s', protocol=%s, use_events=%d, use_vss=%d, routing=%d]\n",
                app_->get_client() , app_->get_name().c_str(),
                (use_tcp_ ? "TCP" : "UDP"),
                use_events_, use_vss_,
                app_->is_routing());

        app_->register_state_handler(
                std::bind(&wiper_client::on_state, this,
                        std::placeholders::_1));

        app_->register_message_handler(
                vsomeip::ANY_SERVICE, vsomeip::ANY_INSTANCE, vsomeip::ANY_METHOD,
//                WIPER_SERVICE_ID, WIPER_INSTANCE_ID, WIPER_METHOD_ID,
                std::bind(&wiper_client::on_message, this,
                        std::placeholders::_1));

        if (use_events_) {
            app_->register_availability_handler(
                    WIPER_SERVICE_ID, WIPER_INSTANCE_ID,
                    std::bind(&wiper_client::on_availability,
                            this,
                            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                    WIPER_SERVICE_MAJOR, WIPER_SERVICE_MINOR);
        }
        if (use_vss_) {
            app_->register_availability_handler(
                    WIPER_VSS_SERVICE_ID, WIPER_VSS_INSTANCE_ID,
                    std::bind(&wiper_client::on_vss_availability,
                            this,
                            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                    WIPER_VSS_SERVICE_MAJOR, WIPER_VSS_SERVICE_MINOR);
        }

        blocked_ = true;
        condition_.notify_one();

        return true;
    }

    // call is blocking current thread
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
        request_condition_.notify_one();

        app_->clear_all_handler();

        if (use_events_) {
            // cleanup event service
            app_->unsubscribe(WIPER_SERVICE_ID, WIPER_INSTANCE_ID, WIPER_EVENTGROUP_ID);
            app_->release_event(WIPER_SERVICE_ID, WIPER_INSTANCE_ID, WIPER_EVENT_ID);
            app_->release_service(WIPER_SERVICE_ID, WIPER_INSTANCE_ID);
        }

        if (use_vss_) {
            // cleanup req/resp service
            app_->release_service(WIPER_VSS_SERVICE_ID, WIPER_VSS_INSTANCE_ID);
        }

        app_->stop();
    }

    void on_state(vsomeip::state_type_e _state) {
        if (_state == vsomeip::state_type_e::ST_REGISTERED) {
            if (use_events_) {
                std::printf("[on_state] Requesting WiperEvent [%04x.%04x] v%u.%u\n",
                        WIPER_SERVICE_ID,
                        WIPER_INSTANCE_ID,
                        WIPER_SERVICE_MAJOR,
                        WIPER_SERVICE_MINOR);
                app_->request_service(
                        WIPER_SERVICE_ID,
                        WIPER_INSTANCE_ID,
                        WIPER_SERVICE_MAJOR,
                        WIPER_SERVICE_MINOR);
            }
            if (use_vss_) {
                std::printf("[on_state] Requesting VSS [%04x.%04x] v%u.%u\n",
                        WIPER_VSS_SERVICE_ID,
                        WIPER_VSS_INSTANCE_ID,
                        WIPER_VSS_SERVICE_MAJOR,
                        WIPER_VSS_SERVICE_MINOR);
                app_->request_service(
                        WIPER_VSS_SERVICE_ID,
                        WIPER_VSS_INSTANCE_ID,
                        WIPER_VSS_SERVICE_MAJOR,
                        WIPER_VSS_SERVICE_MINOR);
            }
            is_registered_ = true;
        }
    }

    void on_availability(vsomeip::service_t _service, vsomeip::instance_t _instance, bool _is_available) {
        std::cout << "### Wiper Event Service ["
                << std::setw(4) << std::setfill('0') << std::hex << _service << "." << _instance
                << "] is "
                << (_is_available ? "available." : "NOT available.")
                << std::endl;

		if (_is_available && use_events_) {
			std::set<vsomeip::eventgroup_t> its_groups;
			its_groups.insert(WIPER_EVENTGROUP_ID);
			app_->request_event(
					WIPER_SERVICE_ID, WIPER_INSTANCE_ID, WIPER_EVENT_ID,
					its_groups, vsomeip::event_type_e::ET_FIELD,
					(use_tcp_ ? vsomeip::reliability_type_e::RT_RELIABLE : vsomeip::reliability_type_e::RT_UNRELIABLE));

			app_->subscribe(WIPER_SERVICE_ID, WIPER_INSTANCE_ID, WIPER_EVENTGROUP_ID, WIPER_SERVICE_MAJOR);
		}
    }

    void on_vss_availability(vsomeip::service_t _service, vsomeip::instance_t _instance, bool _is_available) {
        std::cout << "### VSS Service ["
                << std::setw(4) << std::setfill('0') << std::hex << _service << "." << _instance
                << "] is "
                << (_is_available ? "available." : "NOT available.")
                << std::endl;

		if (_is_available && use_vss_) {
            wiper_vss_set(vss_req_);
		}
    }

    void on_message(const std::shared_ptr<vsomeip::message> &_response) {
        std::stringstream its_message;
        sdv::someip::wiper::t_Event event;

        its_message << "Received a "
                << message_type_to_string(_response->get_message_type()) << " for ["
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

        if (_response->get_service() == WIPER_SERVICE_ID &&
            _response->get_instance() == WIPER_INSTANCE_ID &&
            _response->get_method() == WIPER_EVENT_ID)
        {
            sdv::someip::wiper::deserialize_event(
                    reinterpret_cast<const uint8_t*>(its_payload->get_data()),
                    its_payload->get_length(),
                    event);
            sdv::someip::wiper::print_status("###", event);
        } else
        if (_response->get_service() == WIPER_VSS_SERVICE_ID &&
            _response->get_instance() == WIPER_VSS_INSTANCE_ID &&
            _response->get_method() == WIPER_VSS_METHOD_ID)
        {
            // std::lock_guard<std::mutex> its_lock(request_mutex_);
            // request_status_.code = _response->get_return_code();
            // request_status_.payload = _response->get_payload();
            std::cout << "### Got VSS Reply: { "
                    << "rc:" << std::dec << (int)_response->get_return_code()
                    << ", 0x[ "
                    << sdv::someip::wiper::bytes_to_string(
                            _response->get_payload()->get_data(),
                            _response->get_payload()->get_length())
                    << "] }" << std::endl;
            request_condition_.notify_one();

            if (!use_events_) {
    			std::cout << "### Stopping app (no events)." << std::endl;
			    stop();
		    }

        } else {
            std::cout << "### Got message from unknown service!" << std::endl;
        }
    }

    void run() {
        // request/response thread
        if (debug > 0) std::cout << "// TH: waiting for init..." << std::endl;
        std::unique_lock<std::mutex> its_lock(mutex_);
        while (running_ && !blocked_)
            condition_.wait(its_lock);
        if (debug > 0) std::cout << "// TH: init done." << std::endl;

        int vss_index = 0;
        t_WiperRequest vss_reqests[] = {
            { 40, 80.0f, e_WiperMode::WIPE },
            { 50, 20.0f, e_WiperMode::WIPE },
            { 30, 10.0f, e_WiperMode::WIPE },
            { 30,  2.0f, e_WiperMode::WIPE },
            { 70,  2.0f, e_WiperMode::EMERGENCY_STOP }
        };

        while (running_) {
            // sleep some time, then send vss request
            std::cout << "TH: Sendng VSS[" << vss_index << "] ..." << std::endl;
            bool res = wiper_vss_set(vss_reqests[vss_index++]);
            if (vss_index > 5) {
                vss_index = 0;
            }
            for (int i = 0; i<5 && running_; i++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }

    }

    bool wiper_vss_set(t_WiperRequest vss_reqest, bool wait_response = false) {

        std::unique_lock<std::mutex> its_lock(request_mutex_);

        //  CreatePayload for VSS Request
        uint8_t data[WIPER_SET_PAYLOAD_SIZE];
        if (!sdv::someip::wiper::serialize_vss_request(data, sizeof(data), vss_reqest)) {
            std::cerr << "Failed serializing VSS data!" << std::endl;
            return false;
        }
        // Create a new request
        std::shared_ptr<vsomeip::message> rq = vsomeip::runtime::get()->create_request();
        // Set the VSS service as target of the request
        rq->set_service(WIPER_VSS_SERVICE_ID);
        rq->set_instance(WIPER_VSS_INSTANCE_ID);
        rq->set_method(WIPER_VSS_METHOD_ID);
        // rq->set_interface_version(WIPER_VSS_SERVICE_MAJOR); not needed, set to vsomeip_v3::sd::interface_version(0x1)

        std::shared_ptr<vsomeip::payload> pl = vsomeip::runtime::get()->create_payload();
        pl->set_data(data, sizeof(data));
        rq->set_payload(pl);
        // Send the request to the service. Response will be delivered to the
        // registered message handler
        std::cout << "### Sending VSS Request: "
                << sdv::someip::wiper::vss_request_to_string(vss_reqest)
                << std::endl;
        app_->send(rq);
        std::cout << "### VSS Request sent." << std::endl;

        if (wait_response) {
            if (debug > 0) std::cout << "// waiting for reply..." << std::endl;
            condition_.wait(its_lock);
            if (debug > 0) std::cout << "// waiting for reply..." << std::endl;
        }

        return true;
    }

};


std::string message_type_to_string(vsomeip::message_type_e msg_type) {
    switch (msg_type) {
        case vsomeip::message_type_e::MT_ERROR:
            return "Error";
         case vsomeip::message_type_e::MT_ERROR_ACK:
            return "Error/ack";
         case vsomeip::message_type_e::MT_NOTIFICATION:
            return "Notification";
         case vsomeip::message_type_e::MT_NOTIFICATION_ACK:
            return "Notification/ack";
         case vsomeip::message_type_e::MT_REQUEST:
            return "Request";
         case vsomeip::message_type_e::MT_REQUEST_ACK:
            return "Request/ack";
         case vsomeip::message_type_e::MT_REQUEST_NO_RETURN:
            return "Request/no_ret";
         case vsomeip::message_type_e::MT_REQUEST_NO_RETURN_ACK:
            return "Request/no_ret/ack";
         case vsomeip::message_type_e::MT_RESPONSE:
            return "Response";
         case vsomeip::message_type_e::MT_RESPONSE_ACK:
            return "Response/ack";
        default:
            std::stringstream its_message;
            its_message << "Unknown <0x" << std::hex << (int)msg_type << ">";
            return its_message.str();
    }
}


wiper_client *wiper_client_ptr(nullptr);
void handle_signal(int _signal) {
    if (wiper_client_ptr != nullptr &&
            (_signal == SIGINT || _signal == SIGTERM))
        wiper_client_ptr->stop();
}


int main(int argc, char **argv) {

    // default values
    bool use_tcp = false;
    bool use_vss = false;
    bool use_events = true;

    uint8_t vss_freq = 40;
    float vss_pos = 60.0f;
    e_WiperMode vss_mode = e_WiperMode::WIPE;

    std::string arg_tcp_enable("--tcp");
    std::string arg_udp_enable("--udp");


    std::string arg_vss_mode("--mode");
    std::string arg_vss_freq("--freq");
    std::string arg_vss_pos("--pos");

    std::string arg_vss_only("--vss");

    int i = 1;
    while (i < argc) {
        if (arg_tcp_enable == argv[i]) {
            use_tcp = true;
        } else if (arg_udp_enable == argv[i]) {
            use_tcp = false;
        } else if (arg_vss_only == argv[i]) {
            use_events = false;
            use_vss = true; // fallback to defaults
            if (debug > 1) std::cout << "  // [main] use_events: " << use_events << std::endl;
        } else if (arg_vss_mode == argv[i] && i + 1 < argc) {
            vss_mode = (e_WiperMode)atoi(argv[++i]);
            if (debug > 1) std::cout << "  // [main] vss_mode: " << (int)vss_mode << std::endl;
            use_vss = true;
        } else if (arg_vss_freq == argv[i] && i + 1 < argc) {
            vss_freq = atoi(argv[++i]);
            if (debug > 1) std::cout << "  // [main] vss_freq: " << std::dec << (int)vss_freq << std::endl;
            use_vss = true;
        } else if (arg_vss_pos == argv[i] && i + 1 < argc) {
            vss_pos = atof(argv[++i]);
            if (debug > 1) std::cout << "  // [main] vss_pos: " << vss_pos << std::endl;
            use_vss = true;
        } else {
            std::cout << "Usage: " << argv[0] << " {CONNECTION} {VSS_OPTIONS} " << std::endl;
            std::cout << std::endl;
            std::cout << "CONNECTION:" << std::endl;
            std::cout << "\t --tcp  \tUse reliable Some/IP endpoints" << std::endl;
            std::cout << "\t --udp  \tUse unreliable Some/IP endpoints. Default:true" << std::endl;
            std::cout << std::endl;
            std::cout << "VSS_OPTIONS:" << std::endl;
            std::cout << "\t --vss  \tOnly Set Wiper Mode (no events)" << std::endl;
            std::cout << "\t --mode \tSet Wiper Mode (0=PLANT, 1=STOP, 2=WIPE, 3=EMERGENCY). Default:2" << std::endl;
            std::cout << "\t --freq \tSet Wiper Frequency [0..90], Default:40 " << std::endl;
            std::cout << "\t --pos  \tSet Wiper Position (0.0..180.0). Default:60.0" << std::endl;
            std::cout << std::endl;
            exit(1);
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

    t_WiperRequest vss = { vss_freq, vss_pos, vss_mode };
    if (debug > 1 && use_vss) {
        std::cout << "  // [main] Setting VSS : " << vss_request_to_string(vss) << std::endl;
    }
    wiper_client client(use_tcp, use_events, use_vss, vss);
    wiper_client_ptr = &client;
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    if (client.init()) {
        client.start();
        return 0;
    } else {
        return 1;
    }
}
