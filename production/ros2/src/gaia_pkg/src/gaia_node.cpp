/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gaia_pkg/gaia_node.hpp"

gaia_node::gaia_node(const NodeOptions& options)
: Node("gaia_node", options)
{
    cout << "Hello World!" << endl;

    on_shutdown([&]
    {
        gaia_node::shutdown_callback();
    });

    m_pub_test = this->create_publisher<gaia_pkg_interfaces::msg::Test>(
        "test", ParametersQoS());
}

void gaia_node::shutdown_callback()
{
    cout << "Stopping node." << endl;
}
