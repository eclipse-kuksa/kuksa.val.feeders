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
project(WiperExamples CXX)

# external dependencies
# find_package(Boost 1.55 COMPONENTS system thread filesystem REQUIRED)
if (NOT vsomeip3_POPULATED)
  find_package(vsomeip3)
endif()
find_package(Threads REQUIRED)

# add_definitions(-DVSOMEIP_ENABLE_SIGNAL_HANDLING=1)

# Wiper POC
add_executable(wiper_service
    wiper_server.cc
    wiper_sim.cc
)
target_link_libraries(wiper_service
    wiper_poc
    vsomeip3
    # ${Boost_LIBRARIES}
    # ${DL_LIBRARY}
    pthread
)
target_include_directories(wiper_service
  PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(wiper_client
    wiper_client.cc
)
target_include_directories(wiper_client
  PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(wiper_client
    wiper_poc
    vsomeip3
    # ${Boost_LIBRARIES}
    # ${DL_LIBRARY}
    pthread
)

install(TARGETS
    wiper_service
    wiper_client
    RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
)

###################################################################################################
