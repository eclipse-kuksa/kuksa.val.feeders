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

static int sim_debug = getenv("SIM_DEBUG") ? atoi(getenv("SIM_DEBUG")) : 0;

wiper_simulator::wiper_simulator(uint32_t _cycle) :
    model_counter_(0),
    gen_(rd_()),
    cycle_(_cycle),
    current_rnd_(),
    speed_rnd_(),
    sim_pos_step_(0),
    sim_frequency_(0),
    sim_wiping_(false),
    sim_cylce_ending_(false),
    sim_cycle_ending_count_(0),
    sim_set_ts_(0),
    sim_ts_(0)
{
    model_init();
}

void wiper_simulator::model_init() {

    current_rnd_ = std::uniform_real_distribution<float>(-0.05f, 0.05f);
    speed_rnd_ = std::uniform_real_distribution<float>(0.0f, 0.1f);

    auto now = std::chrono::system_clock::now();
    sim_ts_ = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
    sim_set_ts_ = sim_ts_;
    sim_pos_step_ = 0;
    sim_cycle_ending_count_ = 0;
    sim_target_pos_ = 15;

    event_.sequenceCounter = 0;
    event_.data.ActualPosition = sim_target_pos_;
    event_.data.DriveCurrent = 10;
    event_.data.TempGear = 100;
    event_.data.isBlocked = false;
    event_.data.isEndingWipeCycle = true;
    event_.data.isOverheated = false;
    event_.data.isPositionReached = true;
    event_.data.isWiperError = false;
    event_.data.isWiping = true;
    event_.data.ECUTemp = 75;
    event_.data.LINError = 255;
    event_.data.isUnderVoltage = false;
    event_.data.isOverVoltage = false;
}

void wiper_simulator::model_step(t_Event& event) {

    std::lock_guard<std::mutex> its_lock(configure_mutex_);

    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
    auto sim_elapsed_ms = now_ms - sim_ts_;

    model_counter_++;

    float sim_current = 0.0f;
    float sim_actual_pos = sim_target_pos_;
    bool sim_pos_reached = true;

    // simulate wiper movement (not depending on sim_active_)
    if (sim_wiping_) {
        double sim_moved = sim_elapsed_ms * sim_pos_step_;
        double next_pos = event_.data.ActualPosition + sim_moved;

        bool pos_reached = (sim_pos_step_ >= 0 && next_pos >= sim_target_pos_) ||
                           (sim_pos_step_ < 0 && next_pos <= sim_target_pos_);
        if (pos_reached) {
            // allow several cycle ending events, then pos reached
            if (!sim_cylce_ending_) {
                sim_cylce_ending_ = true;
                sim_cycle_ending_count_ = 0;
            }
            if (++sim_cycle_ending_count_ < 3) {
                next_pos = sim_target_pos_ + speed_rnd_(gen_);
                sim_pos_step_ = 0; // reset sim step
                sim_pos_reached = false;
                if (sim_debug > 0) {
                    std::cout << "[SIM] *** Wiper Cycle ending. current pos: "
                        << next_pos << ", target: " << sim_target_pos_ << std::endl;
                }
            } else {
                if (sim_debug > 0) {
                    auto op_elapsed_ms = now_ms - sim_set_ts_;
                    std::cout << "[SIM] *** Reached target: " << sim_target_pos_
                        << ", sim_position: " << next_pos
                        << " in " << op_elapsed_ms << " ms."
                        << std::endl;
                }
                sim_wiping_ = false;
                sim_cylce_ending_ = false;
                sim_cycle_ending_count_ = 0;
                next_pos = sim_target_pos_;
                sim_pos_reached = true;
            }
        } else {
            sim_pos_reached = false;
        }

        // sanity checks
        if (sim_wiping_ && next_pos < -1.0f) {
            if (sim_debug > 2) {
                std::cout << "[SIM] Reset invalid position " << next_pos << " to 0." << std::endl;
            }
            next_pos = 0;
        }
        if (sim_wiping_ && next_pos > 181.0f) {
            if (sim_debug > 2) {
                std::cout << "[SIM] Reset invalid position: " << next_pos << " to 180."  << std::endl;
            }
            next_pos = 180.0f;
        }

        sim_actual_pos = next_pos; //sim_elapsed_ms * sim_pos_step_; // sim_pos_step_ + speed_rnd_(gen_);
        sim_current = default_current + current_rnd_(gen_);
    }

    event_.sequenceCounter = (uint8_t)(model_counter_ & 0xFF);
    event_.data.isWiping = sim_wiping_;
    event_.data.isPositionReached = sim_pos_reached;
    event_.data.isEndingWipeCycle = sim_cylce_ending_;
    event_.data.DriveCurrent = sim_current;
    event_.data.ActualPosition = sim_actual_pos;

    sim_ts_ = now_ms;
    event = event_; // copy should be ok

    // reduce dumps (each second) if wiping has stopped
    if (sim_debug > 0 && (sim_wiping_ || (model_counter_ % 10) == 0)) {
        std::printf("[SIM] ## ActualPos:%6.2f, DriveCurrent:%5.2f, Wiping:%d, CycEnd:%d, PosReach:%d, Seq:%-3d | sim_model_step:%5.4f, elapsed:%-4ld\n",
                event.data.ActualPosition, event.data.DriveCurrent,
                event.data.isWiping, event.data.isEndingWipeCycle, event.data.isPositionReached,
                (int)event.sequenceCounter, sim_pos_step_, sim_elapsed_ms);
    }
}


void wiper_simulator::model_set(const t_WiperRequest &req) {
    std::lock_guard<std::mutex> its_lock(configure_mutex_);

    std::cout << "[SIM] *** WiperSim::model_set <"
        << wiper_mode_to_string(req.Mode)
        << ", freq:" << (int)req.Frequency
        << ", target:" << req.TargetPosition
        << ">" << std::endl;

    if (req.Mode == e_WiperMode::WIPE) {
        sim_wiping_ = true;
    } else {
        sim_wiping_ = false;
    }

    sim_frequency_ = req.Frequency;
    sim_target_pos_ = req.TargetPosition;
    // | freq = cycles per 60000 ms <=> freq * 180.0 degrees per 60000 ms.
    // | model step per ms
    sim_pos_step_ = sim_frequency_ * 180.0f / 60000.0f;

    // get current pos vs target pos
    if (event_.data.ActualPosition > sim_target_pos_) {
        sim_pos_step_ = -sim_pos_step_;
    }
    if (debug > 0) {
        std::cout << "[SIM] *** Moving ("
                << event_.data.ActualPosition
                << " -> " << sim_target_pos_
                << "), pos/ms: " << sim_pos_step_;
    }

    if (debug > 0) std::cout << std::endl << "[SIM] *** wiping "
        << (sim_wiping_ ? "started." : "stopped.")
        << std::endl << std::endl;
    // reset timestamp
    auto now = std::chrono::system_clock::now();
    sim_set_ts_ = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
    sim_ts_ = sim_set_ts_;
    sim_cylce_ending_ = false;
}

bool wiper_simulator::is_cycle_ending() {
    return sim_cylce_ending_;
}

}  // namespace wiper
}  // namespace someip
}  // namespace sdv
