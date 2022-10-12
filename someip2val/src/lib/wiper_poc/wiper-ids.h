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

#ifndef SOMEIP_WIPER_IDS_H
#define SOMEIP_WIPER_IDS_H

#include <vsomeip/vsomeip.hpp>

#define SAMPLE_SERVICE_ID       0x60d0
#define SAMPLE_INSTANCE_ID      0x0001
#define SAMPLE_METHOD_ID        0x8001

#define SAMPLE_METHOD_ID        0x8001

#define SAMPLE_EVENT_ID         0x8001 //0x8778

#define SAMPLE_EVENTGROUP_ID    0x0064

#define SAMPLE_SERVICE_MAJOR    0x01u
#define SAMPLE_SERVICE_MINOR    0x00u


/**
 * @brief Example Wiper data structure
 */
typedef struct
{
    _Float32        ActualPosition;
    _Float32        DriveCurrent;
    vsomeip::byte_t TempGear;
    vsomeip::byte_t isWiping;
    vsomeip::byte_t isEndingWipeCycle;
    vsomeip::byte_t isWiperError;
    vsomeip::byte_t isPositionReached;
    vsomeip::byte_t isBlocked;
    vsomeip::byte_t isOverheated;
    vsomeip::byte_t ECUTemp;
    vsomeip::byte_t LINError;
    vsomeip::byte_t isOverVoltage;
    vsomeip::byte_t isUnderVoltage;
} t_EventData;


// wiper event structure
typedef struct
{
    vsomeip::byte_t sequenceCounter;
    t_EventData     data;
} t_Event;

#endif // SOMEIP_WIPER_IDS_H
