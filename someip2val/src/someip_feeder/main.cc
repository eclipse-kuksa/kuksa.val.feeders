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
#include <fstream>
#include <string>
#include <thread>

#include <sys/stat.h>
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

    // Optional: Delete all global objects allocated by libprotobuf.
    google::protobuf::ShutdownProtobufLibrary();
}

bool get_file_size(std::string fname, size_t& fsize) {
    struct stat statbuf;
    if (::stat(fname.c_str(), &statbuf) == -1) {
        perror("[get_file_size] Error accessing");
        fsize = 0;
        return false;
    }
    fsize = statbuf.st_size;
    return true;
}



std::string read_file(std::string fname) {
    // FIXME: Check for maximum file size before allocating memory!
    std::fstream fs(fname);
    std::stringstream ss;
    ss << fs.rdbuf();
    return ss.str();
}

void print_help(const char* application) {
    std::cout
        << "Usage: " << application << " <OPTIONS>\n"
        << "\nOPTIONS:\n"
        << "  --target=<ip>:<port>            Databroker address. [Default: localhost:55555].'\n"
        << "  --someip-cfg <config.json>      Specify vsomeip json configuration file.\n"
        << "  --someip-app <ApplicationName>  Specify vsomeip Application name.\n"
        << "  --dummy-feeder                  Feed some dummy data to Databroker and exit.\n"
        << "  --token <FILE>                  Use token from specified file to authorize with Databroker.\n"
        << "  --help                          This message.\n"
        << "\n\nEnvironment variables (if not set by command line arguments):\n"
        << "  BROKER_ADDR               Override Databroker address (host:port)\n"
        << "  BROKER_TOKEN_FILE         Use token from specified file to authorize with Databroker.\n"
        << "  BROKER_TOKEN              Use value as token to authorize with Databroker.\n"
        << "  VSOMEIP_CONFIGURATION     Specify vsomeip json configuration file.\n"
        << "  VSOMEIP_APPLICATION_NAME  Specify vsomeip application name.\n"
        << "  SOMEIP_CLI_DEBUG          SOME/IP Client debug level [0=OFF, 1=INFO, 2=DEBUG, 3=TRACE]\n"
        << "  DBF_DEBUG                 Databroker Feeder debug levels [0=OFF, 1=INFO, 2=DEBUG, 3=TRACE]\n"
        << "  KUKSA_DEBUG               Kuksa/GRPC debug levels [0=OFF, 1=INFO, 2=DEBUG, 3=TRACE]\n"
        << "  WIPER_STATUS              0=disable printing of Wiper event status lines, 1=normal printing (default), 2=same line printing.\n"
        << std::endl;
}

/**
 * @brief main Instantiate the feeder. It requires a channel, out of which the actual RPCs
 * are created. This channel models a connection to an endpoint specified by
 * the argument "--target=".
 * We indicate that the channel isn't authenticated (use of InsecureChannelCredentials()).
 */
int main(int argc, char** argv) {

    std::string target_str = sdv::someip::getEnvironmentStr("BROKER_ADDR", "localhost:55555");
    std::string token_file_str = sdv::someip::getEnvironmentStr("BROKER_TOKEN_FILE", {});
    std::string token_str;
    std::string someip_config;
    std::string someip_app;
    bool use_dummy_feeder = false;

    std::string arg_target("--target");
    std::string arg_someip_cfg("--someip-cfg");
    std::string arg_someip_app("--someip-app");
    std::string arg_dummy_feeder("--dummy-feeder");
    std::string arg_token("--token");
    std::string arg_help("--help");

    // FIXME: update someip settings from command line!
    for (int i = 1; i < argc; i++) {
        std::string arg_val = argv[i];
        if (arg_help == arg_val) {
            print_help(argv[0]);
            exit(0);
        } else if (arg_someip_cfg == arg_val && i < argc - 1) {
            someip_config = argv[++i];
            continue;
        } else if (arg_someip_app == arg_val && i < argc - 1) {
            someip_app = argv[++i];
            continue;
        } else if (arg_token == arg_val && i < argc - 1) {
            token_file_str = argv[++i];
            continue;
        } else {
            size_t start_pos = arg_val.find(arg_target);
            if (start_pos != std::string::npos) {
                start_pos += arg_target.size();
                if (arg_val[start_pos] == '=') {
                    target_str = arg_val.substr(start_pos + 1);
                    continue;
                } else {
                    std::cout << "Target argument syntax is --target=<ip>:<port>" << std::endl;
                    exit(1);
                }
            }
            // fallback for unknown arg / missing option
            std::cerr << "Invalid argument: " << arg_val << std::endl;
            print_help(argv[0]);
            exit(1);
        }
    }

    if (!token_file_str.empty()) {
        // sanity check for token filesize
        size_t token_size;
        if (!get_file_size(token_file_str, token_size)) {
            std::cerr << "Can't read token from: " << token_file_str << std::endl;
            exit(1);
        }
        std::cout << "# Reading token from " << token_file_str << ", size:" << token_size << std::endl;
        if (token_size == 0 || token_size > 16000) {
            std::cerr << "Invliad token file size!" << std::endl;
            exit(1);
        }
        token_str = read_file(token_file_str);
    }

    if (vsomeip::DEFAULT_MAJOR != 0) {
        std::cout << "# Warning: compiled with vsomeip::DEFAULT_MAJOR=" << std::dec << (int)vsomeip::DEFAULT_MAJOR << std::endl;
    }

    // create generic SomeIPClient settings using SOMEIP_CLI_* environment variables (dumps used env vars!)
    sdv::someip::SomeIPConfig config = sdv::someip::SomeIPClient::createEnvConfig();
    // override config with cmdline args
    if (!someip_config.empty()) {
        config.app_config = someip_config.c_str();
        ::setenv("VSOMEIP_CONFIGURATION", someip_config.c_str(), true);
    }
    if (!someip_app.empty()) {
        config.app_name = someip_app.c_str();
        ::setenv("VSOMEIP_APPLICATION_NAME", someip_app.c_str(), true);
    }



    // Initialize Databroker Feeder
    adapter.InitDataBrokerFeeder(target_str, token_str);

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
