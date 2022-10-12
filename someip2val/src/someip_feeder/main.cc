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
// #include <unistd.h> // access()

#include "someip_kuksa_adapter.h"

#include "data_broker_feeder.h"
#include "someip_client.h"
#include "wiper_poc.h"

using sdv::databroker::v1::Datapoint;
using sdv::databroker::v1::DataType;
using sdv::databroker::v1::ChangeType;
using sdv::databroker::v1::Datapoint_Failure;

static sdv::adapter::SomeipFeederAdapter adapter;
static volatile bool adapter_stopping = false;

void handle_signal(int _signal) {
    if (_signal == SIGINT || _signal == SIGTERM) {
        if (!adapter_stopping) {
            adapter_stopping = true; // prevent reentrant calls
            adapter.Shutdown();
        }
    }
}


/**
 * @brief main Instantiate the feeder. It requires a channel, out of which the actual RPCs
 * are created. This channel models a connection to an endpoint specified by
 * the argument "--target=".
 * We indicate that the channel isn't authenticated (use of InsecureChannelCredentials()).
 */
int main(int argc, char** argv) {

    std::string target_str = "localhost:55555";
    bool use_tcp = false;
    bool use_dummy_feeder = false;

    std::string arg_target("--target");
    std::string arg_someip_tcp_enable("--tcp");
    std::string arg_someip_udp_enable("--udp");
    std::string arg_someip_cfg("--someip-cfg");
    std::string arg_someip_app("--someip-app");
    std::string arg_dummy_feeder("--dummy-feeder");

    // Override generic SomeIPClient settings using SOMEIP_CLI_* environment variables:
    sdv::someip::SomeIPConfig config = sdv::someip::SomeIPClient::createEnvConfig();

    // FIXME: update someip settings from command line!
    for (int i = 1; i < argc; i++) {
        std::string arg_val = argv[i];
        if (arg_someip_tcp_enable == arg_val) {
            use_tcp = true;
        } else if (arg_someip_udp_enable == arg_val) {
            use_tcp = false;
        } else if (arg_dummy_feeder == arg_val) {
            use_tcp = false;
        } else if (arg_someip_cfg == arg_val) {
            if (i < argc - 1) {
                std::string arg(argv[++i]);
                if (!arg.empty()) {
                    ::setenv("VSOMEIP_CONFIGURATION", arg.c_str(), true);
                    config.app_config = arg.c_str();
                }
                continue;
            }
        } else if (arg_someip_app == arg_val) {
            if (i < argc - 1) {
                std::string arg(argv[++i]);
                if (!arg.empty()) {
                    ::setenv("VSOMEIP_APPLICATION_NAME", arg.c_str(), true);
                    config.app_name = arg.c_str();
                }
                continue;
            }
        } else {
            size_t start_pos = arg_val.find(arg_target);
            if (start_pos != std::string::npos) {
                start_pos += arg_target.size();
                if (arg_val[start_pos] == '=') {
                    target_str = arg_val.substr(start_pos + 1);
                } else {
                    std::cout << "Target argument syntax is --target=<ip>:<port>" << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Invalid argument: " << arg_val << std::endl;
            }
        }
    }

    // register cleanup signal handler
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Initialize Databroker Feeder
    adapter.initDataBrokerFeeder(target_str);

    // Crete Some/IP client instance, chek required env. variables and fallback to dummy feeder on problems
    if (!adapter.initSomeipClient(config)) {
        std::cout << "SOME/IP not available. feeding some dummy data..." << std::endl;
        use_dummy_feeder = true;
    }

    if (use_dummy_feeder) {
        adapter.FeedDummyData();
    }

    // Runs both databroker feeder and someip threads
    adapter.Run();

    // Stop both databroker and someip instances and join threads
    adapter.Shutdown();

    return 0;
}