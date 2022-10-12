#********************************************************************************
# Copyright (c) 2022 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License 2.0 which is available at
# http://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
#*******************************************************************************/

cmake_minimum_required (VERSION 3.11)

# Project
project(SomeIp2ValFeeder CXX)

# external dependencies
find_package(vsomeip3 REQUIRED)
find_package(Threads REQUIRED)

find_package(gRPC REQUIRED)
find_package(Protobuf REQUIRED)

add_executable(someip_feeder
  "someip_kuksa_adapter.cc"
  "main.cc"
)

target_link_libraries(someip_feeder PRIVATE
  data_broker_feeder
  someip_client
  wiper_poc
)

install(
  TARGETS someip_feeder
  RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
)

