/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_components/register_node_macro.hpp"

#include "gaia_incubator/msg/temp.hpp"
#include "gaia_incubator/msg/fan_state.hpp"
#include "gaia_incubator/msg/add_incubator.hpp"
#include "gaia_incubator/msg/add_sensor.hpp"
#include "gaia_incubator/msg/add_fan.hpp"

using namespace std;
using namespace rclcpp;
using namespace gaia_incubator;

class gaia_logic : public Node
{
public:
    gaia_logic(const NodeOptions& options);

private:
    void temp_sensor_callback(const msg::Temp::SharedPtr msg);

    void shutdown_callback();

    void setup_incubators();

    Subscription<msg::Temp>::SharedPtr m_sub_temp;
};

RCLCPP_COMPONENTS_REGISTER_NODE(gaia_logic)
