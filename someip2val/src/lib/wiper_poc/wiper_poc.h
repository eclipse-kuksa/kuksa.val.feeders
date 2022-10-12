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

#ifndef VSOMEIP_WIPER_POC_H
#define VSOMEIP_WIPER_POC_H

#include <cstring>
#include <sstream>
#include <iomanip>

/////////////////////////////
// someip wiper service ids
/////////////////////////////

#define WIPER_SERVICE_ID       0x60D0
#define WIPER_INSTANCE_ID      0x0001
#define WIPER_METHOD_ID        0x8001

#define WIPER_EVENT_ID         0x8001

#define WIPER_EVENTGROUP_ID    0x0064

#define WIPER_SERVICE_MAJOR    0x01
#define WIPER_SERVICE_MINOR    0x00


namespace sdv {
namespace someip {
namespace wiper {


typedef struct
{
    _Float32    ActualPosition;
    _Float32    DriveCurrent;
    uint8_t     TempGear;
    uint8_t     isWiping;
    uint8_t     isEndingWipeCycle;
    uint8_t     isWiperError;
    uint8_t     isPositionReached;
    uint8_t     isBlocked;
    uint8_t     isOverheated;
    uint8_t     ECUTemp;
    uint8_t     LINError;
    uint8_t     isOverVoltage;
    uint8_t     isUnderVoltage;
} t_EventData;


// wiper event structure
typedef struct
{
    uint8_t     sequenceCounter;
    t_EventData data;
} t_Event;


void float_to_bytes(float val, uint8_t* data);
void bytes_to_float(const uint8_t* data, float* val);

std::string event_to_string(const t_Event &event);
std::string bytes_to_string(const uint8_t *payload, size_t payload_size);

bool deserialize_event(const uint8_t *payload, size_t payload_size, t_Event& event);

void print_status(const std::string &prefix, const t_Event &event);


}  // namespace wiper
}  // namespace someip
}  // namespace sdv


#endif // VSOMEIP_WIPER_POC_H
