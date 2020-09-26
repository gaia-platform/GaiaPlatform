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

    /*template<typename T_msg>
    void @TABLE_NAME@_insert(const typename T_msg::SharedPtr msg)
    {
        // basic scalar types
        auto @TABLE_NAME@_writer = @TABLE_NAME@_writer();
        @TABLE_NAME@_writer.@FIELD_NAME@ = msg.@FIELD_NAME@; // N times
        auto @TABLE_NAME@ = @TABLE_NAME@::get(@TABLE_NAME@_writer.insert_row());
        // END basic scalar types

        // basic array types
        auto @TABLE_NAME@__@FIELD_NAME@_writer = @TABLE_NAME@__@FIELD_NAME@_writer();
        for (int i = 0; i < msg.arr_static.size(); ++i)
        {
            @TABLE_NAME@__@FIELD_NAME@_writer.index = i;
            @TABLE_NAME@__@FIELD_NAME@_writer.value = msg.@FIELD_NAME@[i];
            @TABLE_NAME@.@TABLE_NAME@__@FIELD_NAME@_list().insert(@TABLE_NAME@__@FIELD_NAME@_writer.insert_row());
        }
        // END basic array types

        // complex scalar types
        auto @TABLE_NAME@__@FIELD_NAME@_writer = @TABLE_NAME@__@FIELD_NAME@_writer();
        @TABLE_NAME@__@FIELD_NAME@_writer.@NESTED_FIELD_NAME@ = msg.@FIELD_NAME@.@NESTED_FIELD_NAME@; // N times
        @TABLE_NAME@.@TABLE_NAME@__@FIELD_NAME@_list().insert(@TABLE_NAME@__@FIELD_NAME@_writer.insert_row());
        // END complex scalar types

        // complex array types
        auto test__msg_foo_writer = test__msg_foo_writer();
        for (int i = 0; i < msg.msg_foo.size(); ++i)
        {
            test__msg_foo_writer.index = i;
            test__msg_foo_writer.foo_int = msg.msg_foo[i];
            test__msg_foo = test__msg_foo::get(test__msg_foo_writer.insert_row());

            auto test__msg_foo__temp_writer = test__msg_foo__temp_writer();
            test__msg_foo__temp_writer.temperature = msg.msg_foo[i].temperature;
            test__msg_foo__temp_writer.variance = msg.msg_foo[i].variance;
            test__msg_foo.test__msg_foo__temp_list.insert(test__msg_foo__temp_writer.insert_row());

            test.test__msg_foo_list().insert(test__msg_foo_writer.insert_row());
        }
        // END complex array types
    }

    template<typename T_msg>
    void @TABLE_NAME@_callback(const typename T_msg::SharedPtr msg)
    {
        auto_transaction tx;
        @TABLE_NAME@_insert<T_msg::SharedPtr>(msg);
    }*/

    template<typename T_msg>
    void subscriber_callback(const typename T_msg::SharedPtr msg)
    {
        (void) msg;
        /*auto_transaction tx;

        // basic scalar types
        auto test_writer = test_writer();
        test_writer.test_int = msg.test_int;
        test_writer.test_float = msg.test_float;
        test_writer.test_string = msg.test_string;
        test_writer.str_bounded = msg.str_bounded;
        auto test = test::get(test_writer.insert_row());
        // END basic scalar types

        // basic array types
        auto test__arr_static_writer = test__arr_static_writer();
        for (int i = 0; i < msg.arr_static.size(); ++i)
        {
            test__arr_static_writer.index = i;
            test__arr_static_writer.value = msg.arr_static[i];
            test.test__arr_static_list().insert(test__arr_static_writer.insert_row());
        }

        auto test__arr_bounded_writer = test__arr_bounded_writer();
        for (int i = 0; i < msg.arr_bounded.size(); ++i)
        {
            test__arr_bounded_writer.index = i;
            test__arr_bounded_writer.value = msg.arr_bounded[i];
            test.test__arr_bounded_list().insert(test__arr_bounded_writer.insert_row());
        }
        // END basic array types

        // complex scalar types
        auto test__stamp_writer = test__stamp_writer();
        test__stamp_writer.sec = msg.stamp.sec;
        test__stamp_writer.nanosec = msg.stamp.nanosec;
        test.test__stamp_list().insert(test__stamp_writer.insert_row());
        // END complex scalar types

        // complex array types
        auto test__msg_foo_writer = test__msg_foo_writer();
        for (int i = 0; i < msg.msg_foo.size(); ++i)
        {
            test__msg_foo_writer.index = i;
            test__msg_foo_writer.foo_int = msg.msg_foo[i];
            test__msg_foo = test__msg_foo::get(test__msg_foo_writer.insert_row());

            auto test__msg_foo__temp_writer = test__msg_foo__temp_writer();
            test__msg_foo__temp_writer.temperature = msg.msg_foo[i].temperature;
            test__msg_foo__temp_writer.variance = msg.msg_foo[i].variance;
            test__msg_foo.test__msg_foo__temp_list.insert(test__msg_foo__temp_writer.insert_row());

            test.test__msg_foo_list().insert(test__msg_foo_writer.insert_row());
        }
        // END complex array types
        */
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
