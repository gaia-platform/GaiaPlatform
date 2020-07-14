/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// gaia includes
/*#include "barn_storage_gaia_generated.h"
#include "events.hpp"
#include "gaia_system.hpp"
#include "rules.hpp"*/

#include <memory>
#include <functional>

#include "gaia_incubator/gaia_logic.hpp"

// gaia namespaces
/*using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::rules;
using namespace BarnStorage;*/

gaia_logic::gaia_logic(const NodeOptions& options)
: Node("gaia_logic", options)
{
    cout << "Starting gaia_logic." << endl;
    on_shutdown([&]{gaia_logic::shutdown_callback();});

    using std::placeholders::_1;

    m_sub_temp =
        this->create_subscription<msg::Temp>(
            "temp", SystemDefaultsQoS(), std::bind(
                &gaia_logic::temp_sensor_callback, this, _1));
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

// gaia-specific function
/*void init_storage() {
    begin_transaction();

    ulong gaia_id;

    gaia_id = Incubator::insert_row("test_incubator", 90.0, 100.0);
    Sensor::insert_row(gaia_id, "test_sensor", 0, 99.0);
    Actuator::insert_row(gaia_id, "test_fan", 0, 0.0);

    commit_transaction();
}*/
