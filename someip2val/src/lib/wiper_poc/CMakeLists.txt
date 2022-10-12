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

project(wiper_poc)

cmake_minimum_required(VERSION 3.11)

# Wiper POC
add_library(wiper_poc
  EXCLUDE_FROM_ALL
  STATIC
    wiper_poc.cc
)

target_include_directories(wiper_poc
  PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS wiper_poc
  PRIVATE_HEADER
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)