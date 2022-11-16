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

#ifndef VSOMEIP_WIPER_SIM_H
#define VSOMEIP_WIPER_SIM_H

#include <random>
#include <mutex>

#include "wiper_poc.h"

namespace sdv {
namespace someip {
namespace wiper {

const float default_pos_step = 3.13f;
const float default_current = 10.0f;

class wiper_simulator {
public:
    wiper_simulator(uint32_t _cycle);
    
    void model_init();

    void model_step(t_Event& event);

    void model_set(const t_WiperRequest &req);

private:
    int model_counter_;
    uint32_t cycle_;
    t_Event event_;

    float sim_pos_step_;
    bool  sim_wiping_;
    int   sim_frequency_;
    float sim_target_pos_;
    float sim_model_step_;
    
    std::random_device rd_; // Will be used to obtain a seed for the random number engine
    std::mt19937 gen_;
    std::uniform_real_distribution<float> current_rnd_;
    std::uniform_real_distribution<float> speed_rnd_;
    
    std::mutex configure_mutex_;
};

}  // namespace wiper
}  // namespace someip
}  // namespace sdv


#endif // VSOMEIP_WIPER_SIM_H
