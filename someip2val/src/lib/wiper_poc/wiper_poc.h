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
#include <vector>

/////////////////////////////
// someip wiper service ids
/////////////////////////////

#define WIPER_SERVICE_ID        0x60D0
#define WIPER_INSTANCE_ID       0x0001
#define WIPER_METHOD_ID         0x8001
#define WIPER_EVENT_ID          0x8001
#define WIPER_EVENTGROUP_ID     0x0064
#define WIPER_SERVICE_MAJOR     0x01
#define WIPER_SERVICE_MINOR     0x00

#define WIPER_VSS_SERVICE_ID    0x6123
#define WIPER_VSS_INSTANCE_ID   0x000b
#define WIPER_VSS_METHOD_ID     0x0007
#define WIPER_VSS_SERVICE_MAJOR 0x01
#define WIPER_VSS_SERVICE_MINOR 0x00


namespace sdv {
namespace someip {
namespace wiper {

#define WIPER_EVENT_PAYLOAD_SIZE    20
#define WIPER_SET_PAYLOAD_SIZE      6

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


/**
 * @brief Payload structure for Wiper Events
 */
typedef struct
{
    uint8_t     sequenceCounter;
    t_EventData data;
} t_Event;

typedef enum
{
    PLANT_MODE      = 0,
    STOP_HOLD       = 1,
    WIPE            = 2,
    EMERGENCY_STOP  = 3
} e_WiperMode;

/**
 * @brief Payload structure for Wiper Set service
 */
typedef struct
{
    uint8_t     Frequency;
    _Float32    TargetPosition;
    e_WiperMode Mode;
} t_WiperRequest;

void float_to_bytes(float val, uint8_t* data);
void bytes_to_float(const uint8_t* data, float* val);

/*************************************/
/**       Wiper Event service       **/
/*************************************/
std::string event_to_string(const t_Event &event);
std::string bytes_to_string(const uint8_t *payload, size_t payload_size);

bool deserialize_event(const uint8_t *payload, size_t payload_size, t_Event& event);
bool serialize_wiper_event(const t_Event& event, uint8_t *payload, size_t payload_size);

// print with new line
void print_status(const std::string &prefix, const t_Event &event);
// print on the same line
void print_status_r(const std::string &prefix, const t_Event &event);

/*************************************/
/**  Wiper Request service helpers  **/
/*************************************/

bool serialize_vss_request(uint8_t *payload, size_t payload_size, const t_WiperRequest &request);
bool deserialize_vss_request(const uint8_t *payload, size_t payload_size, t_WiperRequest &request);
bool wiper_mode_parse(const std::string &str, e_WiperMode &mode);
std::string wiper_mode_to_string(e_WiperMode mode);
std::string vss_request_to_string(const t_WiperRequest &request);


}  // namespace wiper
}  // namespace someip
}  // namespace sdv


#endif // VSOMEIP_WIPER_POC_H
