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

#include "wiper_sim.h"

extern int debug;

namespace sdv {
namespace someip {
namespace wiper {

wiper_simulator::wiper_simulator(uint32_t _cycle, bool _sim_active) :
    model_counter_(0),
    gen_(rd_()),
    cycle_(_cycle),
    current_rnd_(),
    speed_rnd_(),
    sim_pos_step_(1),
    sim_frequency_(0),
    sim_model_step_(0),
    sim_wiping_(false),
    sim_active_(_sim_active)
{
}

void wiper_simulator::model_init() {

    //std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    // std::uniform_real_distribution<float> current_rnd(-0.05f, 0.05f);
    // std::uniform_real_distribution<float> speed_rnd(0.0f, default_pos_step/3.0f);

    current_rnd_ = std::uniform_real_distribution<float>(-0.05f, 0.05f);
    speed_rnd_ = std::uniform_real_distribution<float>(0.0f, default_pos_step/3.0f);

    sim_pos_step_ = default_pos_step;

    event_.sequenceCounter = 0;
    event_.data.ActualPosition = 15;
    event_.data.DriveCurrent = 10;
    event_.data.TempGear = 100;
    event_.data.isBlocked = false;
    event_.data.isEndingWipeCycle = false;
    event_.data.isOverheated = false;
    event_.data.isPositionReached = false;
    event_.data.isWiperError = false;
    event_.data.isWiping = true;
    event_.data.ECUTemp = 75;
    event_.data.LINError = 255;
    event_.data.isUnderVoltage = false;
    event_.data.isOverVoltage = false;
}

void wiper_simulator::model_step(t_Event& event) {

    std::lock_guard<std::mutex> its_lock(configure_mutex_);

    model_counter_++;
    event_.sequenceCounter = (uint8_t)(model_counter_ & 0xFF);

    // only change states if simulation is active, otherwise just react to vss set commands
    if (sim_active_) {
        // toggle isOverheated ~9s
        if ((cycle_ * model_counter_) % 9000 == 0) {
            event_.data.isOverheated = !event_.data.isOverheated;
            std::cout << std::endl << "*** wiper "
                << (event_.data.isOverheated ? "Overheated." : "not Overheated") << std::endl << std::endl;
        }

        // toggle isWiping ~5s
        if ((cycle_ * model_counter_) % 15000 == 0) {
            sim_wiping_ = !sim_wiping_;
            std::cout << std::endl << "*** wiping "
                << (event_.data.isWiping ? "started." : "stopped.") << std::endl << std::endl;
        }
    }
    event_.data.isWiping = sim_wiping_;

    // simulate wiper movement (not depending on sim_active_)
    if (sim_wiping_) {
        // assume positive
        if (event_.data.ActualPosition >= sim_target_pos_) {
            sim_pos_step_ = -default_pos_step;
        } else
        if (event_.data.ActualPosition < default_pos_step) {
            sim_pos_step_ = default_pos_step;
        }
        event_.data.ActualPosition += sim_pos_step_ + speed_rnd_(gen_);

        event_.data.DriveCurrent = default_current + current_rnd_(gen_);
    } else {
        event_.data.DriveCurrent = 0.0f;
    }

    event = event_; // copy should be ok
}


void wiper_simulator::model_set(const t_WiperRequest &req) {
    std::lock_guard<std::mutex> its_lock(configure_mutex_);

    std::cout << "*** WiperSim::model_set <"
        << wiper_mode_to_string(req.Mode)
        << ", freq:" << (int)req.Frequency
        << ", target:" << req.TargetPosition
        << ">" << std::endl;

    if (req.Mode == e_WiperMode::WIPE) {
        sim_wiping_ = true;
    } else {
        sim_wiping_ = false;
    }

    if (debug > 0) std::cout << std::endl << "*** wiping "
        << (sim_wiping_ ? "started." : "stopped.")
        << std::endl << std::endl;


    sim_frequency_ = req.Frequency;
    sim_target_pos_ = req.TargetPosition;
    // | freq = cycles per 60000 ms <=> freq * 180.0 degrees per 60000 ms.
    // | deg_per_ms = (freq * 120.0) / 60000.0
    // | model_step_per_cycle = cycle_ * deg_per_ms
    sim_model_step_ = cycle_ * sim_frequency_ * 120.0f / 60000.0f;

    // get current pos vs target pos
    if (event_.data.ActualPosition > sim_target_pos_) {
        sim_model_step_ = -sim_model_step_;
    }
    if (debug > 1) std::cout << "   --> sim_model_step_ = " << sim_pos_step_;
}

}  // namespace wiper
}  // namespace someip
}  // namespace sdv