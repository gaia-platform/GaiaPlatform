/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// gaia includes
#include "barn_storage_gaia_generated.h"
#include "events.hpp"
#include "gaia_system.hpp"
#include "rules.hpp"

#include <memory>
#include <functional>
#include <cstdio>
#include <atomic>

#include "gaia_incubator/gaia_logic.hpp"
#include "gaia_incubator/demo_constants.hpp"

// gaia namespaces
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::rules;
using namespace BarnStorage;
using namespace demo_constants;

gaia_logic::gaia_logic(const NodeOptions& options)
: Node("gaia_logic", options)
{
    cout << "Starting gaia_logic." << endl;
    on_shutdown([&]{gaia_logic::shutdown_callback();});

    gaia::system::initialize(true);

    using std::placeholders::_1;

    m_sub_temp = this->create_subscription<msg::Temp>(
        "temp", ParametersQoS(), std::bind(
            &gaia_logic::temp_sensor_callback, this, _1));

    setup_devices();
}

void gaia_logic::temp_sensor_callback(const msg::Temp::SharedPtr msg)
{
    begin_transaction();

    Sensor_data* s_data = new Sensor_data();
    s_data->set_incubator_id(msg->incubator_id);
    s_data->set_name(msg->sensor_name.c_str());
    s_data->set_timestamp(msg->stamp.sec);
    s_data->set_value(msg->value);

    s_data->insert_row();
    commit_transaction();
}

void gaia_logic::shutdown_callback()
{
    cout << "Shut down gaia_logic." << endl;
}

void gaia_logic::setup_devices()
{
    begin_transaction();
    Incubator* puppy_incubator = new Incubator();
    puppy_incubator->set_name("Puppy Incubator");
    puppy_incubator->set_min_temp(85.0);
    puppy_incubator->set_max_temp(85.5);
    puppy_incubator->insert_row();
    gaia_id_t puppy_incubator_id = puppy_incubator->gaia_id();

    Sensor* puppy_sensor1 = new Sensor();
    puppy_sensor1->set_name("Puppy Sensor 1");
    puppy_sensor1->set_incubator_id(puppy_incubator_id);
    puppy_sensor1->insert_row();

    Sensor* puppy_sensor2 = new Sensor();
    puppy_sensor2->set_name("Puppy Sensor 2");
    puppy_sensor2->set_incubator_id(puppy_incubator_id);
    puppy_sensor2->insert_row();

    Fan* puppy_fan = new Fan();
    puppy_fan->set_name("Puppy Fan");
    puppy_fan->set_incubator_id(puppy_incubator_id);
    puppy_fan->insert_row();

    commit_transaction();
}

/** ruleset*/
namespace gaia_incubator_ruleset
{
atomic<uint32_t> latest_timestamp{0};

auto ruleset_node = Node::make_shared("ruleset_node");

auto ruleset_pub_fan_speed = ruleset_node->create_publisher<msg::FanSpeed>(
    "fan_speed", ParametersQoS());

auto ruleset_pub_add_incubator
    = ruleset_node->create_publisher<msg::AddIncubator>(
    "add_incubator", ParametersQoS());

auto ruleset_pub_add_sensor = ruleset_node->create_publisher<msg::AddSensor>(
    "add_sensor", ParametersQoS());

auto ruleset_pub_add_fan = ruleset_node->create_publisher<msg::AddFan>(
    "add_fan", ParametersQoS());

void change_fan_speed(Incubator* inc, bool speed_up)
{
    for (unique_ptr<Fan> f{Fan::get_first()}; f; f.reset(f->get_next()))
    {
        if(f->incubator_id() == inc->gaia_id())
        {
            float new_speed;
            if(speed_up)
            {
                new_speed = min(c_fan_max_speed,
                    f->speed() + c_fan_acceleration * c_publish_temp_rate);
            }
            else
            {
                new_speed = max((float) 0.0,
                    f->speed() - c_fan_acceleration * c_publish_temp_rate);
            }

            // Only change the speed if it is not going to be set to its
            // previous speed.
            if(new_speed != f->speed())
            {
                f->set_speed(new_speed);
                f->update_row();
            }
        }
    } // for (all fans)
} // change_fan_speed()

/**
 rule-sensor_data_inserted: [BarnStorage::Sensor_data](insert)
*/
void on_sensor_data_inserted(const rule_context_t* context)
{
    Sensor_data* s_data = Sensor_data::get_row_by_id(context->record);
    Incubator* inc = Incubator::get_row_by_id(s_data->incubator_id());

    if (s_data->timestamp() != latest_timestamp)
    {
        latest_timestamp = s_data->timestamp();
        if (s_data->value() < inc->min_temp())
        {
            change_fan_speed(inc, false);
        }
        else if (s_data->value() > inc->max_temp())
        {
            change_fan_speed(inc, true);
        }
    }

    printf("%-6s|%6ld|%7.1lf\n",
        s_data->name(), s_data->timestamp(), s_data->value());
    fflush(stdout);
}

/**
 rule-incubator_inserted: [BarnStorage::Incubator](insert)
*/
void on_incubator_inserted(const rule_context_t* context)
{
    Incubator* inc = Incubator::get_row_by_id(context->record);

    msg::AddIncubator add_incubator_msg;
    add_incubator_msg.incubator_id = inc->gaia_id();
    add_incubator_msg.name = inc->name();

    ruleset_pub_add_incubator->publish(add_incubator_msg);
}

/**
 rule-sensor_inserted: [BarnStorage::Sensor](insert)
*/
void on_sensor_inserted(const rule_context_t* context)
{
    Sensor* sens = Sensor::get_row_by_id(context->record);

    msg::AddSensor add_sensor_msg;
    add_sensor_msg.sensor_name = sens->name();
    add_sensor_msg.incubator_id = sens->incubator_id();

    ruleset_pub_add_sensor->publish(add_sensor_msg);
}

/**
 rule-fan_inserted: [BarnStorage::Fan](insert)
*/
void on_fan_inserted(const rule_context_t* context)
{
    Fan* f = Fan::get_row_by_id(context->record);

    msg::AddFan add_fan_msg;
    add_fan_msg.fan_name = f->name();
    add_fan_msg.incubator_id = f->incubator_id();

    ruleset_pub_add_fan->publish(add_fan_msg);
}

/**
 rule-fan_updated: [BarnStorage::Fan](update)
*/
void on_fan_updated(const rule_context_t* context)
{
    Fan* f = Fan::get_row_by_id(context->record);

    msg::FanSpeed fan_speed_msg;
    fan_speed_msg.speed = f->speed();
    fan_speed_msg.fan_name = f->name();
    fan_speed_msg.incubator_id = f->incubator_id();

    ruleset_pub_fan_speed->publish(fan_speed_msg);
}
}
