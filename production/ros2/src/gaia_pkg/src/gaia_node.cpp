/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_components/register_node_macro.hpp"

#include "gaia_pkg_interfaces/msg/test.hpp"

using namespace std;
using namespace rclcpp;

class gaia_node : public Node
{
public:
    gaia_node(const NodeOptions& options) : Node("gaia_node", options)
    {
        cout << "Hello World!" << endl;

        on_shutdown([&]
        {
            shutdown_callback();
        });
    }

private:
    void shutdown_callback()
    {
        cout << "Stopping node." << endl;
    }

    template<typename T_msg>
    void basic_scalar_insert(const typename T_msg::SharedPtr msg)
    {
        (void) msg;
    }

    template<typename T_msg>
    void subscriber_callback(const typename T_msg::SharedPtr msg)
    {
        (void) msg;
    }

    template<typename T_msg>
    void generate_subscriber(const char* topic_name)
    {
        typename Subscription<T_msg>::SharedPtr sub = this->create_subscription<T_msg>(
            topic_name, SystemDefaultsQoS(),
            [&](const typename T_msg::SharedPtr msg)
            {
                this->subscriber_callback<T_msg>(msg);
            }
        );
    }
};

RCLCPP_COMPONENTS_REGISTER_NODE(gaia_node)
