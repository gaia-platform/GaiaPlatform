/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_components/register_node_macro.hpp"

using namespace std;
using namespace rclcpp;

class gaia_node : public Node
{
public:
    gaia_node(const NodeOptions& options);

private:
    void shutdown_callback();
};

RCLCPP_COMPONENTS_REGISTER_NODE(gaia_node)
