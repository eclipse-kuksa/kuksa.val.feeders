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

# Project
project(SomeIp2Val CXX)

# use conan as dependency management
include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

###########################################
### vsomeip support as cmake dependency ###
###########################################

# workaround for outdated cmake code in vsomeip handling Boost_VERSION
# include (FindBoost)
find_package(Boost 1.55 COMPONENTS system thread filesystem REQUIRED)
if(Boost_FOUND)
  MESSAGE(NOTICE "Boost_VERSION: ${Boost_VERSION}")
  if (NOT DEFINED Boost_VERSION_MACRO)
    string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" BOOST_VERSION_MATCH ${Boost_VERSION})
    MATH(EXPR detected_boost_ver "${CMAKE_MATCH_1} * 100000 + ${CMAKE_MATCH_2} * 100 + ${CMAKE_MATCH_3}" )
    # set(Boost_VERSION_MACRO 107200)
    set(Boost_VERSION_MACRO ${detected_boost_ver})
    MESSAGE(STATUS "Boost_VERSION_MACRO set to: ${Boost_VERSION_MACRO}")
  endif()
else()
  MESSAGE(WARNING "Boost was not found!")
endif()

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# build vsomeip from sources, no connan support
include(FetchContent)

# we want signal handling enabled for vsomeip...
# add_definitions(-DVSOMEIP_ENABLE_SIGNAL_HANDLING=1)

MESSAGE(STATUS "FetchContent: vsomeip...")
FetchContent_Declare(
  vsomeip3
  GIT_REPOSITORY    https://github.com/COVESA/vsomeip.git
  GIT_TAG           master
  # GIT_TAG           3.1.20.3
  PATCH_COMMAND     git apply --ignore-whitespace
                        "${CMAKE_SOURCE_DIR}/patches/000-cmake-reduce-gcc-warnings.patch"
                        "${CMAKE_SOURCE_DIR}/patches/001-vsomeip-logger-fixes.patch"
                        "${CMAKE_SOURCE_DIR}/patches/002-vsomeip-default-major-change.patch"
                        # "${CMAKE_SOURCE_DIR}/patches/003-vsomeip-application-debugs.patch"
  GIT_SHALLOW       ON
  GIT_PROGRESS      TRUE
)
FetchContent_MakeAvailable(vsomeip3)

# exclude vsomeip files from installation, commented as we need built libraries
# if(faIS_DIRECTORY "${vsomeip3_SOURCE_DIR}")
#   message(STATUS "Excluding vsomeip3 sources: ${vsomeip3_SOURCE_DIR}")
#   set_property(DIRECTORY ${vsomeip3_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL YES)
# endif()

# add sources after vsomeip3 is populated
add_subdirectory(src)

### copy scripts from current source dir to current bin dir
configure_file(./bin/setup-someip2val.sh ${CMAKE_CURRENT_BINARY_DIR}/ COPYONLY)
configure_file(./bin/setup-someip2val-proxy.sh ${CMAKE_CURRENT_BINARY_DIR}/ COPYONLY)
configure_file(./bin/setup-wiper-service.sh ${CMAKE_CURRENT_BINARY_DIR}/ COPYONLY)
configure_file(./config/someip_wiper_service.json ${CMAKE_CURRENT_BINARY_DIR}/config/ COPYONLY)
configure_file(./config/someip_wiper_client-proxy.json ${CMAKE_CURRENT_BINARY_DIR}/config/ COPYONLY)

###
### install scripts and config
###
install(PROGRAMS
  ./bin/setup-someip2val-proxy.sh
  ./bin/setup-someip2val.sh
  ./bin/setup-wiper-service.sh
  DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
)

install(FILES
  ./config/someip_wiper_client.json
  ./config/someip_wiper_client-proxy.json
  ./config/someip_wiper_service.json
  DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/config"
)
