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

    gaia::system::initialize(true);

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
    begin_transaction();

    Sensor::insert_row(msg->incubator_id, msg->sensor_name.c_str(),
        msg->stamp.sec, msg->value);

    commit_transaction();
}

void gaia_logic::shutdown_callback()
{
    dump_db();
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

void gaia_logic::dump_db()
{
    begin_transaction();

    for (unique_ptr<Sensor> s{Sensor::get_first()}; s;
         s.reset(s->get_next()))
    {
        printf("%-6s|%6ld|%7.1lf\n", s->name(), s->timestamp(),
            s->value());
    }

    for (unique_ptr<Actuator> a{Actuator::get_first()}; a;
         a.reset(a->get_next()))
    {
        printf("%-6s|%6ld|%7.1lf\n", a->name(), a->timestamp(),
            a->value());
    }

    commit_transaction();
    fflush(stdout);
}

namespace incubator_ruleset
{
/**
 rule-sensor_inserted: [BarnStorage::Sensor](insert)
*/
void on_sensor_inserted(const rule_context_t *context)
{
    Sensor* s = Sensor::get_row_by_id(context->record);
    // TODO: try writing to a file
    // TODO: implement initialize_rules()
    printf("%s\n", s->name());
}

}
