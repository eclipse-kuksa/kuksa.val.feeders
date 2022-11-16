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

#include <unistd.h>
#include <fcntl.h>

#include "someip_kuksa_adapter.h"

#include "data_broker_feeder.h"
#include "someip_client.h"
#include "wiper_poc.h"

#define SELF "[main] "

using sdv::databroker::v1::Datapoint;
using sdv::databroker::v1::DataType;
using sdv::databroker::v1::ChangeType;
using sdv::databroker::v1::Datapoint_Failure;

static sdv::adapter::SomeipFeederAdapter adapter;

// Self pipe (for signal handling)
int pipefd[2];

void signal_handler(int signal) {
    char sig = signal;
    while (write(pipefd[1], &sig, sizeof(sig)) < 0) {
        if (errno == EINTR) {
            // If write was interupted by a signal, try again.
        } else {
            // Otherwise it doesn't make much sense to try again.
            break;
        }
    }
}

int setup_signal_handler() {
    // Setup signal handler (using a self pipe)
    if (pipe(pipefd) == -1) {
        std::cout << SELF "Failed to setup signal handler (self pipe)" << std::endl;
        std::exit(1);
    }

    auto pipe_read_fd = pipefd[0];
    auto pipe_write_fd = pipefd[1];

    // Set write end of pipe to non-blocking in order for it to work reliably in
    // the signal handler. We _do_ want the read end to remain blocking so we
    // can block while waiting for a signal.
    int flags = fcntl(pipe_write_fd, F_GETFL) | O_NONBLOCK;
    if (fcntl(pipe_write_fd, F_SETFL, flags) != 0) {
        std::cout << SELF "Failed to set self pipe to non blocking" << std::endl;
        std::exit(1);
    }
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    return pipe_read_fd;  // return read end of pipe
}

// Blocks until signal is received
void wait_for_signal(int fd) {
    char buf;
    auto res = read(fd, &buf, sizeof(buf));
    if (res < 0) {
        perror(SELF"[wait_for_signal] read() error");
    } else if (res == 1) {
        std::cerr << SELF "[wait_for_signal] Received signal: " << std::dec << (int) buf << std::endl;
    } else {
        std::cerr << SELF "[wait_for_signal] unexpected EOF" << std::endl;
    }
}

void AdapterRun() {
    // Setup signal handler & wait for signal
    auto fd = setup_signal_handler();

    // std::atexit(atExitHandler);

    // Runs both databroker feeder and someip threads
    adapter.Start();

    std::cout << std::endl << SELF "Running adapter... (Press Ctrl+C to stop.)" << std::endl << std::endl;
    wait_for_signal(fd);

    std::cerr << std::endl << std::endl;
    std::cerr << SELF "Shutting down from signal handler.." << std::endl;
    adapter.Shutdown();
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

    if (vsomeip::DEFAULT_MAJOR != 0) {
        std::cout << "# Warning: compiled with vsomeip::DEFAULT_MAJOR=" << std::dec << (int)vsomeip::DEFAULT_MAJOR << std::endl;
    }
    // Initialize Databroker Feeder
    adapter.InitDataBrokerFeeder(target_str);

    // Crete Some/IP client instance, chek required env. variables and fallback to dummy feeder on problems
    if (!adapter.InitSomeipClient(config)) {
        std::cout << "SOME/IP not available. feeding some dummy data..." << std::endl;
        use_dummy_feeder = true;
    }
    if (use_dummy_feeder) {
        // no sighandler for dummy feeder...
        adapter.FeedDummyData();
    } else {
        AdapterRun();
    }
    return 0;
}
