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

#include <iostream>

#include <stdint.h>
#include "wiper_poc.h"

namespace sdv {
namespace someip {
namespace wiper {


void float_to_bytes(float val, uint8_t* data) {
    // Create union of shared memory space
    union {
        float   float_variable;
        uint8_t temp_array[4];
    } u;
    // Overite byte s of union with float variable
    u.float_variable = val;
    // Assign bytes to input array
    memcpy(data, u.temp_array, 4);
}

void bytes_to_float(const uint8_t* data, float* val) {
    // Create union of shared memory space
    union {
        float   float_variable;
        uint8_t temp_array[4];
    } u;

    // Overite float of union with byte array
    memcpy(u.temp_array, data, 4);
    // copy union float to val
    *val = u.float_variable;
}

bool deserialize_event(const uint8_t *payload, size_t payload_size, t_Event& event) {
    uint8_t tmp[4];

    memset(&event, 0, sizeof(event));
    if (payload_size < 20) {
        std::cerr << __func__ << " Payload size " << payload_size << " is too small!" << std::endl;
        return false;
    }
    event.sequenceCounter = payload[0];

    tmp[0] = payload[1];
    tmp[1] = payload[2];
    tmp[2] = payload[3];
    tmp[3] = payload[4];
    bytes_to_float(tmp, &event.data.ActualPosition);

    tmp[0] = payload[5];
    tmp[1] = payload[6];
    tmp[2] = payload[7];
    tmp[3] = payload[8];
    bytes_to_float(tmp, &event.data.DriveCurrent);

    event.data.TempGear             = payload[9];
    event.data.isWiping             = payload[10];
    event.data.isEndingWipeCycle    = payload[11];
    event.data.isWiperError         = payload[12];
    event.data.isPositionReached    = payload[13];
    event.data.isBlocked            = payload[14];
    event.data.isOverheated         = payload[15];
    event.data.ECUTemp              = payload[16];
    event.data.LINError             = payload[17];
    event.data.isOverVoltage        = payload[18];
    event.data.isUnderVoltage       = payload[19];
    return true;
}

std::string bytes_to_string(const uint8_t *payload, size_t payload_size) {
    std::stringstream ss;
    for (uint32_t i = 0; i < payload_size; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int) payload[i] << " ";
    }
    return ss.str();
}

std::string event_to_string(const t_Event &event) {
    std::stringstream ss;
    ss << "WiperEvent: {" << std::endl;
    ss << "  sequenceCounter: 0x"
        << std::hex << std::setw(2) << std::setfill('0')
        << (int) event.sequenceCounter << std::endl;
    ss << "  ActualPosition: "
        << event.data.ActualPosition << std::endl;
    ss << "  DriveCurrent: "
        << event.data.DriveCurrent << std::endl;
    ss << "  ECUTemp: "
        << std::dec << (int) event.data.ECUTemp << std::endl;
    ss << "  isBlocked: "
        << (bool) event.data.isBlocked << std::endl;
    ss << "  isEndingWipeCycle: "
        << (bool) event.data.isEndingWipeCycle << std::endl;
    ss << "  isOverheated: "
        << (bool) event.data.isOverheated << std::endl;
    ss << "  isPositionReached: "
        << (bool) event.data.isPositionReached << std::endl;
    ss << "  isWiperError: "
        << (bool) event.data.isWiperError << std::endl;
    ss << "  isWiping: "
        << (bool) event.data.isWiping << std::endl;
    ss << "  LINError: "
        << (int) event.data.LINError << std::endl;
    ss << "  TempGear: "
        << (int) event.data.TempGear << std::endl;
    ss << "  isOverVoltage: "
       << (bool) event.data.isOverVoltage << std::endl;
    ss << "  isUnderVoltage: "
       << (bool) event.data.isUnderVoltage << std::endl;
    ss << "}" << std::endl;
    return ss.str();
}

void print_status(const std::string &prefix, const t_Event &event) {
    std::printf(
            "%s|WiperEvent| Pos:%8.4f, DC:%5.2f, Wiping:%d, CycEnd:%d PosReach:%d, "
            "Block:%d, Err:%d, LinErr:%d, EcuTmp:%02x, GearTmp:%02x, Seq:%02x\n",
				prefix.c_str(),
                event.data.ActualPosition,
                event.data.DriveCurrent,
                event.data.isWiping,
                event.data.isEndingWipeCycle,
                event.data.isPositionReached,

                event.data.isBlocked,
                event.data.isWiperError,
                event.data.LINError,

                event.data.ECUTemp,
                event.data.TempGear,
                event.sequenceCounter);
}


}  // namespace wiper
}  // namespace someip
}  // namespace sdv
