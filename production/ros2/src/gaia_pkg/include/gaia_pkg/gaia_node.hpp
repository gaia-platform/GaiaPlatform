/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_components/register_node_macro.hpp"

#include "gaia_pkg/msg/test.hpp"

using namespace std;
using namespace rclcpp;
using namespace gaia_pkg;

class gaia_node : public Node
{
public:
    gaia_node(const NodeOptions& options);

private:
    void shutdown_callback();

    Publisher<msg::Test>::SharedPtr m_pub_test;
};

RCLCPP_COMPONENTS_REGISTER_NODE(gaia_node)
