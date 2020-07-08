#pragma once

#include "rclcpp/rclcpp.hpp"

#include "gaia_incubator/msg/temp.hpp"

using namespace std;
using namespace rclcpp;
using namespace gaia_incubator;

class gaia_logic : public Node
{
public:
    gaia_logic();

private:
    void temp_sensor_callback(const msg::Temp::SharedPtr msg);

    Subscription<msg::Temp>::SharedPtr m_sub_temp;
};
