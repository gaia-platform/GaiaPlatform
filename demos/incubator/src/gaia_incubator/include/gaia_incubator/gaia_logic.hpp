#pragma once

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_components/register_node_macro.hpp"

#include "gaia_incubator/msg/temp.hpp"

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

    Subscription<msg::Temp>::SharedPtr m_sub_temp;
};

RCLCPP_COMPONENTS_REGISTER_NODE(gaia_logic)
