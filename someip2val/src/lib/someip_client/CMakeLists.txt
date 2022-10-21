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

cmake_minimum_required(VERSION 3.11)

# external dependencies
#find_package(Boost 1.55 COMPONENTS system thread filesystem REQUIRED )
if (NOT vsomeip3_POPULATED)
  find_package(vsomeip3)
endif()
find_package(Threads REQUIRED)

# Wiper POC
add_library(someip_client
  EXCLUDE_FROM_ALL
  STATIC
    someip_client.cc
)

target_link_libraries(someip_client
  PRIVATE
    vsomeip3
    ${Boost_LIBRARIES}
    ${DL_LIBRARY}
)

target_include_directories(someip_client
  PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS someip_client
  PUBLIC_HEADER
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

###################################################################################################
