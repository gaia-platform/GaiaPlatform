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

#include "gaia_incubator/gaia_logic.hpp"

// gaia namespaces
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::rules;
using namespace BarnStorage;

gaia_logic::gaia_logic(const NodeOptions& options)
: Node("gaia_logic", options)
{
    cout << "Starting gaia_logic." << endl;
    on_shutdown([&]{gaia_logic::shutdown_callback();});

    gaia::system::initialize(true);

    using std::placeholders::_1;

    m_sub_temp = this->create_subscription<msg::Temp>(
        "temp", SystemDefaultsQoS(), std::bind(
            &gaia_logic::temp_sensor_callback, this, _1));

    setup_incubators();
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

void gaia_logic::setup_incubators()
{
    begin_transaction();

    Incubator* puppy_incubator = new Incubator();
    puppy_incubator->set_name("Puppy Incubator");
    puppy_incubator->set_min_temp(85.0);
    puppy_incubator->set_max_temp(85.5);
    puppy_incubator->insert_row();
    gaia_id_t puppy_incubator_id = puppy_incubator->gaia_id();

    Sensor* puppy_sensor = new Sensor();
    puppy_sensor->set_name("Puppy Sensor");
    puppy_sensor->set_incubator_id(puppy_incubator_id);
    puppy_sensor->insert_row();

    Fan* puppy_fan = new Fan();
    puppy_fan->set_name("Puppy Fan");
    puppy_fan->set_incubator_id(puppy_incubator_id);
    puppy_fan->insert_row();

    commit_transaction();
}

/** ruleset*/
namespace gaia_incubator_ruleset
{
auto ruleset_node = Node::make_shared("ruleset_node");

auto ruleset_pub_fan_state = ruleset_node->create_publisher<msg::FanState>(
    "fan_state", SystemDefaultsQoS());

auto ruleset_pub_add_incubator
    = ruleset_node->create_publisher<msg::AddIncubator>(
    "add_incubator", SystemDefaultsQoS());

auto ruleset_pub_add_sensor = ruleset_node->create_publisher<msg::AddSensor>(
    "add_sensor", SystemDefaultsQoS());

auto ruleset_pub_add_fan = ruleset_node->create_publisher<msg::AddFan>(
    "add_fan", SystemDefaultsQoS());

/**
 rule-sensor_data_inserted: [BarnStorage::Sensor_data](insert)
*/
void on_sensor_data_inserted(const rule_context_t *context)
{
    Sensor_data* s_data = Sensor_data::get_row_by_id(context->record);
    Incubator* inc = Incubator::get_row_by_id(s_data->incubator_id());

    msg::FanState fan_state_msg;
    fan_state_msg.incubator_id = inc->gaia_id();

    bool should_publish_fan_state = false;

    if (s_data->value() < inc->min_temp())
    {
        fan_state_msg.fan_on = false;
        should_publish_fan_state = true;
    }
    else if (s_data->value() > inc->max_temp())
    {
        fan_state_msg.fan_on = true;
        should_publish_fan_state = true;
    }

    if(should_publish_fan_state)
    {
        for (unique_ptr<Fan> f{Fan::get_first()}; f; f.reset(f->get_next()))
        {
            if (f->incubator_id() == inc->gaia_id())
            {
                fan_state_msg.fan_name = f->name();
                ruleset_pub_fan_state->publish(fan_state_msg);
            }
        }
    }

    printf("%-6s|%6ld|%7.1lf\n",
        s_data->name(), s_data->timestamp(), s_data->value());
    fflush(stdout);
}

/**
 rule-incubator_inserted: [BarnStorage::Incubator](insert)
*/
void on_incubator_inserted(const rule_context_t *context)
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
void on_sensor_inserted(const rule_context_t *context)
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
void on_fan_inserted(const rule_context_t *context)
{
    Fan* f = Fan::get_row_by_id(context->record);

    msg::AddFan add_fan_msg;
    add_fan_msg.fan_name = f->name();
    add_fan_msg.incubator_id = f->incubator_id();

    ruleset_pub_add_fan->publish(add_fan_msg);
}
}
