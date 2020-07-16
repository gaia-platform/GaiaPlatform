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

    using std::placeholders::_1;

    m_pub_add_incubator = this->create_publisher<msg::AddIncubator>(
        "add_incubator", SystemDefaultsQoS());

    m_pub_add_sensor = this->create_publisher<msg::AddSensor>(
        "add_sensor", SystemDefaultsQoS());

    m_pub_add_fan = this->create_publisher<msg::AddFan>(
        "add_fan", SystemDefaultsQoS());

    m_sub_temp = this->create_subscription<msg::Temp>(
        "temp", SystemDefaultsQoS(), std::bind(
            &gaia_logic::temp_sensor_callback, this, _1));

    setup_incubators();
}

void gaia_logic::temp_sensor_callback(const msg::Temp::SharedPtr msg)
{
    cout << msg->sensor_name << ": " << msg->stamp.sec
    << " | " << msg->value << endl;
}

void gaia_logic::shutdown_callback()
{
    cout << "Shut down gaia_logic." << endl;
}

void gaia_logic::setup_incubators()
{
    msg::AddIncubator add_incubator_msg;
    add_incubator_msg.incubator_id = 1;
    add_incubator_msg.name = "Puppy";
    m_pub_add_incubator->publish(add_incubator_msg);

    msg::AddSensor add_sensor_msg;
    add_sensor_msg.sensor_name = "Puppy Sensor";
    add_sensor_msg.incubator_id = 1;
    m_pub_add_sensor->publish(add_sensor_msg);

    msg::AddFan add_fan_msg;
    add_fan_msg.fan_name = "Puppy Fan";
    add_fan_msg.incubator_id = 1;
    m_pub_add_fan->publish(add_fan_msg);
}

// An unused function for checking Gaia dependencies at compile-time.
void gaia_logic::init_storage() {
    begin_transaction();
    commit_transaction();
}
