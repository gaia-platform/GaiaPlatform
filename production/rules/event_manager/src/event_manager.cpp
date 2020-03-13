/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "event_manager.hpp"

using namespace gaia::events;
using namespace gaia::rules;
using namespace gaia::common;

/**
 * Class implementation
 */
event_manager_t& event_manager_t::get()
{
    static event_manager_t s_instance;
    return s_instance;
}

event_manager_t::event_manager_t() 
{
}

event_manager_t::~event_manager_t()
{
}

bool event_manager_t::log_event(event_type type, event_mode mode)
{
    return false;
}

bool event_manager_t::log_event(
    gaia_base_t* row, 
    event_type type, 
    event_mode mode)
{
    return false;
}

bool event_manager_t::subscribe_rule(
    gaia_type_t gaia_type, 
    event_type type, 
    rule_binding_t& rule_binding)
{
    return false;
}

bool event_manager_t::subscribe_rule(
    event_type type, 
    rule_binding_t& rule_binding)
{
    return false;
}
bool event_manager_t::unsubscribe_rule(
    gaia_type_t gaia_type, 
    event_type type, 
    const rule_binding_t& rule_binding)
{
    return false;
}

bool event_manager_t::unsubscribe_rule(
    event_type type, 
    const gaia::rules::rule_binding_t& rule_binding)
{
    return false;
}

void event_manager_t::list_subscribed_rules(
    const char* ruleset_name, 
    const gaia_type_t* gaia_type, 
    const event_type* type,
    std::vector<const char *>& rule_names)
{
}

/**
 * Public event API implementation
 */
bool gaia::events::log_transaction_event(event_type type, event_mode mode)
{
    return event_manager_t::get().log_event(type, mode);
}

bool gaia::events::log_table_event(
    gaia_base_t* row, 
    event_type type, 
    event_mode mode)
{
    return event_manager_t::get().log_event(row, type, mode);
}

/**
 * Public rules API implementation
 */
bool gaia::rules::subscribe_table_rule(
    gaia_type_t gaia_type, 
    event_type type, 
    rule_binding_t& rule_binding)
{
    return event_manager_t::get().subscribe_rule(gaia_type, type, rule_binding);
}

bool gaia::rules::subscribe_transaction_rule(
    event_type type, 
    gaia::rules::rule_binding_t& rule_binding)
{
    return event_manager_t::get().subscribe_rule(type, rule_binding);
}

bool gaia::rules::unsubscribe_table_rule(
    gaia_type_t gaia_type, 
    event_type type, 
    const gaia::rules::rule_binding_t& rule_binding)
{
    return event_manager_t::get().unsubscribe_rule(gaia_type, type, rule_binding);
}

bool gaia::rules::unsubscribe_transaction_rule(
    event_type type, 
    const gaia::rules::rule_binding_t& rule_binding)
{
    return event_manager_t::get().unsubscribe_rule(type, rule_binding);
}
    
void gaia::rules::list_subscribed_rules(
    const char* ruleset_name, 
    const gaia_type_t* gaia_type, 
    const event_type* type, 
    std::vector<const char *>& rule_names)
{
    event_manager_t::get().list_subscribed_rules(ruleset_name, gaia_type,
        type, rule_names);
}
