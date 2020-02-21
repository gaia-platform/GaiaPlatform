/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "event_manager.h"

bool event_manager::init()
{
}

bool event_manager::log_event(on_row_change event, event_mode mode)
{
}

bool event_manager::log_event(on_tx_commit event)
{
}

//
// problem here is that we need to load in the context the rule expects
// i.e., bind to event, sensor_changed (column change of sensor's table)
// but I also want the incubator limits to know what to do.
// rule looks something like:
/*
rule_1(float sensor_value, incubator * chicken_incubator)
{
    if (sensor_value > chicken_incubator.upper) {
        chicken_incubator.fan++;
    }
    else
    if (sensor_value < chicken_incubator.lower) {
        chicken_incubator.fan--;
    }
}

// reference the one I care about, note that this prototype is
// generated

// rule binds to a single event
// rule binds to multiple events
// multiople rules bind to a single event
// multiple rules bind to multiple events

/**
 * rule binds to single event
 * 
 * @event[update sensors] // row change
 * 
 * @event[insert sensors] // row added to sensors table
 * 
 * @event[update sensors](value, timestamp) // column(s) change
 * 
 * @event[on_row_change](sensor)
 */
bool rule_1(rule_context& ctx)
{
}

/**
 * rule binds to single event
 * 
 * @event[update sensors]  // column change
 * 
 * @event[insert sensors]
 * 
 * @event[change sensors](value, timestamp)
 * 
 * @event[on_column_change](sensor.timestamp)
 * 
 * @event[on_row_change](sensor)
 */
bool rule_1(rule_context& ctx)
{
}




