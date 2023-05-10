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
#pragma once

#include <cstdlib>
#include <iostream>
#include <string>

namespace sdv {
namespace logger {

const int LEVEL_ERR = 0;
const int LEVEL_INF = 1;
const int LEVEL_DBG = 2;
const int LEVEL_TRC = 3;

#define DEFAULT_LOGLEVEL    ::sdv::logger::LEVEL_INF

/// Using static C strings as structs are destroyed atexit() and logging causes invalid reads

/**
 * @brief Use LOGGER_STATIC_INIT("<module-prefix>") in your class *before* namespace declaration
 */
#define LOGGER_STATIC_INIT(MODULE) \
        using namespace sdv::logger; \
        static constexpr const char* g_log_module = MODULE; \
        static int g_log_level = DEFAULT_LOGLEVEL;

#define LOGGER_SET_LEVEL(LEVEL) \
    g_log_level = LEVEL;

#define LOGGER_SET_LEVEL_ENV(ENV_PROP, DEFAULT_VALUE) \
    g_log_level = ::sdv::logger::GetEnvironmentInt(ENV_PROP, DEFAULT_VALUE);

#define LOGGER_ENABLED(LEVEL) \
    (g_log_level >= LEVEL)


// /!\ WARNING: LOG_XXX functions use unsafe if (), could consume else from parent block!
#define LOG_TRACE   if (LOGGER_ENABLED(::sdv::logger::LEVEL_TRC)) std::cout << g_log_module << __func__ << ": [trace] "
#define LOG_DEBUG   if (LOGGER_ENABLED(::sdv::logger::LEVEL_DBG)) std::cout << g_log_module << __func__ << ": [debug] "
#define LOG_INFO    if (LOGGER_ENABLED(::sdv::logger::LEVEL_INF)) std::cout << g_log_module << __func__ << ": [info] "
#define LOG_ERROR   if (LOGGER_ENABLED(::sdv::logger::LEVEL_ERR)) std::cerr << g_log_module << __func__ << ": [error] "

inline int GetEnvironmentInt(std::string envName, int default_value) {
    const char* val = std::getenv(envName.c_str());
    if (!val) return default_value;
    return std::atoi(val);
}

}  // namespace logger
}  // namespace sdv
