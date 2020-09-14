/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_components/register_node_macro.hpp"

#include "gaia_pkg_interfaces/msg/test.hpp"

using namespace std;
using namespace rclcpp;

class gaia_node : public Node
{
public:
    gaia_node(const NodeOptions& options);

private:
    void shutdown_callback();

    Publisher<gaia_pkg_interfaces::msg::Test>::SharedPtr m_pub_test;
};

RCLCPP_COMPONENTS_REGISTER_NODE(gaia_node)
