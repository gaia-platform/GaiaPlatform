/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "event_manager.hpp"

#include <cstring>
#include "event_guard.hpp"

using namespace gaia::rules;
using namespace gaia::common;
using namespace std;

/**
 * Class implementation
 */
event_manager_t& event_manager_t::get(bool is_initializing)
{
    static event_manager_t s_instance;

    // Initialize errors can happen for two reasons:
    // 
    // If we are currently trying to initialize then is_initializing 
    // will be true. At this point, we don't expect the instance to be 
    // initialized yet.
    //
    // If we are not intializing then we expect the instance to already be
    // initialized
    if (is_initializing == s_instance.m_is_initialized)
    {
        throw initialization_error(is_initializing);
    }
    return s_instance;
}

event_manager_t::event_manager_t() 
{
}

void event_manager_t::init()
{
    // Placeholder for code that must execute before the event manager 
    // is used to log events or invoke rules.
    m_is_initialized = true;
}



bool event_manager_t::log_event(event_type_t event_type, event_mode_t mode)
{
    bool rules_fired = false;

    check_mode(mode);
    check_transaction_event(event_type);

    // Don't allow reentrant log_event calls.
    event_guard_t guard(m_log_events[0], event_type);
    if (guard.is_blocked())
    {
        return rules_fired;
    }

    // Invoke all rules immediately.
    rule_list_t& rules = m_transaction_subscriptions[event_type];
    rules_fired = rules.size() > 0;

    // Log the event to the database
    log_to_db(0, event_type, mode, nullptr, rules_fired);
    
    for (auto rules_it = rules.begin(); rules_it != rules.end(); ++rules_it) 
    {
        _rule_binding_t* rule_ptr = const_cast<_rule_binding_t*>(*rules_it);
        transaction_context_t context(
            {rule_ptr->ruleset_name.c_str(), rule_ptr->rule_name.c_str(), rule_ptr->rule},
            event_type);
        rule_ptr->rule(&context);
    }

    return rules_fired;
}

bool event_manager_t::log_event(
    gaia_base_t * row, 
    gaia_type_t gaia_type,
    event_type_t event_type, 
    event_mode_t mode)
{
    bool rules_fired = false;

    check_mode(mode);
    check_table_event(event_type);

    // Don't allow reentrant log_event calls.
    event_guard_t guard(m_log_events[gaia_type], event_type);
    if (guard.is_blocked())
    {
        return rules_fired;
    }
    
    // Invoke all rules bound to this gaia_type and event_type immediately.
    auto type_it = m_table_subscriptions.find(gaia_type);
    if (type_it != m_table_subscriptions.end()) 
    {
        events_map_t& events = type_it->second;
        rule_list_t& rules = events[event_type];
        rules_fired = rules.size() > 0;

        log_to_db(gaia_type, event_type, mode, row, rules_fired);

        for (auto rules_it = rules.begin(); rules_it != rules.end(); ++rules_it) 
        {
            _rule_binding_t* rule_ptr = const_cast<_rule_binding_t*>(*rules_it);
            table_context_t context(
                {rule_ptr->ruleset_name.c_str(), rule_ptr->rule_name.c_str(), rule_ptr->rule},
                event_type, 
                gaia_type, 
                row);
            rule_ptr->rule(&context);
        }
    }
    else 
    {
        log_to_db(gaia_type, event_type, mode, row, rules_fired);
    }

    return rules_fired;
}

void event_manager_t::subscribe_rule(
    gaia_type_t gaia_type, 
    event_type_t event_type, 
    const rule_binding_t& rule_binding)
{
    // We only support table events scoped to a gaia_type for now.
    check_table_event(event_type);
    check_rule_binding(rule_binding);

    // If we've bound a rule to this gaia_type before then get
    // its event map. Otherwise, create the event map.
    auto type_it = m_table_subscriptions.find(gaia_type);
    if (type_it == m_table_subscriptions.end()) {
        auto sub_it = m_table_subscriptions.insert(
            make_pair(gaia_type, event_manager_t::create_table_event_map()));
        type_it = sub_it.first;
    }

    // Retreive the rule_list associated with
    // the event_type we are binding to.
    events_map_t& events = type_it->second;
    rule_list_t& rules = events[event_type];
    add_rule(rules, rule_binding);
}

void event_manager_t::subscribe_rule(
    event_type_t event_type, 
    const rule_binding_t& rule_binding)
{
    check_transaction_event(event_type);
    check_rule_binding(rule_binding);

    // If we've haven't bound any rules yet then the events
    // map will be empty.  Lazily create it
    // its event map. Otherwise, create the event map
    if (0 == m_transaction_subscriptions.size()) 
    {
        insert_transaction_events(m_transaction_subscriptions);
    }

    // Retreive the rule_list associated with
    // the event_type we are binding to.
    rule_list_t& rules = m_transaction_subscriptions[event_type];
    add_rule(rules, rule_binding);
}

bool event_manager_t::unsubscribe_rule(
    gaia_type_t gaia_type, 
    event_type_t event_type, 
    const rule_binding_t& rule_binding)
{
    bool removed_rule = false;

    check_table_event(event_type);
    check_rule_binding(rule_binding);

    // If we haven't seen any subscriptions for this type
    // then no rule was bound.
    auto type_it = m_table_subscriptions.find(gaia_type);
    if (type_it != m_table_subscriptions.end()) 
    {
        // Get our rule list for the specific event specified
        // in the rule binding.
        events_map_t& events = type_it->second;
        rule_list_t& rules = events[event_type];
        removed_rule = remove_rule(rules, rule_binding);
    }

    return removed_rule;
}

bool event_manager_t::unsubscribe_rule(
    event_type_t event_type, 
    const rule_binding_t& rule_binding)
{
    check_transaction_event(event_type);
    check_rule_binding(rule_binding);

    rule_list_t& rules = m_transaction_subscriptions[event_type];
    return remove_rule(rules, rule_binding);
}

void event_manager_t::unsubscribe_rules()
{
    m_transaction_subscriptions.clear();
    m_table_subscriptions.clear();
    m_rules.clear();
}


void event_manager_t::list_subscribed_rules(
    const char* ruleset_name, 
    const gaia_type_t* gaia_type_ptr, 
    const event_type_t* type_ptr,
    list_subscriptions_t& subscriptions)
{
    subscriptions.clear();

    // filter first by gaia_type, then by event_type, then by ruleset_name
    for (auto table_it : m_table_subscriptions)
    {
        if (gaia_type_ptr != nullptr && table_it.first != *gaia_type_ptr)
        {
            continue;
        }

        auto events = table_it.second;
        add_subscriptions(subscriptions, events, table_it.first, ruleset_name, type_ptr);
    }

    // If the user didn't specify a gaia_type filter then
    // add transaction event subscriptions.  Again, filter
    // by event_type then by ruleset_name
    if (gaia_type_ptr == nullptr)
    {
        add_subscriptions(subscriptions, m_transaction_subscriptions, 
            0, ruleset_name, type_ptr);
    }
}

void event_manager_t::add_subscriptions(list_subscriptions_t& subscriptions, 
    const events_map_t& events, 
    gaia_type_t gaia_type,
    const char* ruleset_filter, 
    const event_type_t* event_filter)
{
    for (auto event_it : events)
    {
        if (event_filter && event_it.first != *event_filter) 
        {
            continue;
        }

        const rule_list_t& rules = event_it.second;
        for (auto rule : rules) 
        {
            if (ruleset_filter
                && (0 != strcmp(ruleset_filter, rule->ruleset_name.c_str())))
            {
                continue;
            }

            subscriptions.push_back(unique_ptr<subscription_t>(new subscription_t({
                rule->ruleset_name.c_str(),
                rule->rule_name.c_str(),
                gaia_type, event_it.first})));
        }
    }
}

void event_manager_t::add_rule(
    rule_list_t& rules,
    const rule_binding_t& binding)
{
    // Don't allow adding a rule that has the same
    // key as another rule but is bound to a different
    // rule function.
    const _rule_binding_t* rule_ptr = find_rule(binding);
    if (rule_ptr != nullptr && rule_ptr->rule != binding.rule) 
    {
        throw duplicate_rule(binding, true);
    }

    // Dont' allow the caller to bind the same rule to the same rule list.  
    // This is most likely a programming error.
    for (auto rules_it = rules.begin(); rules_it != rules.end(); ++rules_it)
    {
        if (*rules_it == rule_ptr) 
        {
            throw duplicate_rule(binding, false);
        }
    }

    // If we already have seen this rule, then
    // add it to the list.  Otherwise, create a new 
    // rule binding entry and put it in our global list.
    _rule_binding_t* this_rule = nullptr;
    if (rule_ptr == nullptr) 
    {
        const string& key = make_rule_key(binding);
        this_rule = new _rule_binding_t(binding);
        m_rules.insert(make_pair(key, unique_ptr<_rule_binding_t>(this_rule)));
    }
    else 
    {
        this_rule = const_cast<_rule_binding_t*>(rule_ptr);
    }

    // Add the rule to the subscription list.
    rules.push_back(this_rule);
}

bool event_manager_t::remove_rule(
    rule_list_t& rules,
    const rule_binding_t& binding)
{
    bool removed_rule = false;
    const _rule_binding_t* rule_ptr = find_rule(binding);

    if (rule_ptr)
    {
        auto size = rules.size();
        rules.remove_if([&] (const _rule_binding_t* ptr)
        {
            return (ptr == rule_ptr);
        });
        removed_rule = (size != rules.size());
    }

    return removed_rule;
}

const event_manager_t::_rule_binding_t* event_manager_t::find_rule(const rule_binding_t& binding)
{
    auto rule_it = m_rules.find(make_rule_key(binding));
    if (rule_it != m_rules.end()){
        return rule_it->second.get();
    }

    return nullptr;
}

std::string event_manager_t::make_rule_key(const rule_binding_t& binding)
{
    string rule_key = binding.ruleset_name;
    rule_key.append(binding.rule_name);
    return rule_key;
}

event_manager_t::events_map_t event_manager_t::create_table_event_map() {
    return {
        make_pair(event_type_t::column_change, list<const _rule_binding_t*>()),
        make_pair(event_type_t::row_insert, list<const _rule_binding_t*>()),
        make_pair(event_type_t::row_update, list<const _rule_binding_t*>()),
        make_pair(event_type_t::row_delete, list<const _rule_binding_t*>())
    };
}

void event_manager_t::log_to_db(gaia_type_t gaia_type, 
    event_type_t event_type, 
    event_mode_t event_mode,
    gaia_base_t * context,
    bool rules_fired)
{
    static_assert(sizeof(uint32_t) == sizeof(event_type_t), 
        "event_type_t needs to be sizeof uint32_t");
    static_assert(sizeof(uint8_t) == sizeof(event_mode_t), 
        "event_mode_t needs to be sizeof uint8_t");

    uint64_t timestamp = (uint64_t)time(NULL);
    const char * event_source = "";
    gaia_id_t context_id = 0;

    if (context)
    {
        event_source = context->gaia_typename();
        context_id = context->gaia_id();
    }
    Event_log::insert_row((uint64_t)gaia_type, (uint32_t)event_type, 
        (uint8_t) event_mode, event_source, timestamp, context_id, rules_fired);
}

void event_manager_t::insert_transaction_events(event_manager_t::events_map_t& transaction_map) {
    transaction_map.insert(make_pair(event_type_t::transaction_begin, list<const _rule_binding_t*>()));
    transaction_map.insert(make_pair(event_type_t::transaction_commit, list<const _rule_binding_t*>()));
    transaction_map.insert(make_pair(event_type_t::transaction_rollback, list<const _rule_binding_t*>()));
}

// Enable conversion from rule_binding_t -> internal_rules_binding_t.
event_manager_t::_rule_binding_t::_rule_binding_t(
    const rule_binding_t& binding)
{
    ruleset_name = binding.ruleset_name;
    rule = binding.rule;
    if (binding.rule_name != nullptr) 
    {
        rule_name = binding.rule_name;
    }
}

/**
 * Public event API implementation
 */
bool gaia::rules::log_transaction_event(event_type_t type, event_mode_t mode)
{
    return event_manager_t::get().log_event(type, mode);
}

bool gaia::rules::log_table_event(
    gaia_base_t* row, 
    gaia_type_t gaia_type,
    event_type_t type, 
    event_mode_t mode)
{
    return event_manager_t::get().log_event(row, gaia_type, type, mode);
}

/**
 * Public rules API implementation
 */

void gaia::rules::initialize_rules_engine()
{
    bool is_initializing = true;
    event_manager_t::get(is_initializing).init();
    /**
     * This function must be provided by the 
     * rules application.  This function is
     * generated by the gaia preprocessor on
     * behalf of the user.
     */
    initialize_rules();
}

void gaia::rules::subscribe_table_rule(
    gaia_type_t gaia_type, 
    event_type_t type, 
    const rule_binding_t& rule_binding)
{
    event_manager_t::get().subscribe_rule(gaia_type, type, rule_binding);
}

void gaia::rules::subscribe_transaction_rule(
    event_type_t type, 
    const gaia::rules::rule_binding_t& rule_binding)
{
    event_manager_t::get().subscribe_rule(type, rule_binding);
}

bool gaia::rules::unsubscribe_table_rule(
    gaia_type_t gaia_type, 
    event_type_t type, 
    const gaia::rules::rule_binding_t& rule_binding)
{
    return event_manager_t::get().unsubscribe_rule(gaia_type, type, rule_binding);
}

bool gaia::rules::unsubscribe_transaction_rule(
    event_type_t type, 
    const gaia::rules::rule_binding_t& rule_binding)
{
    return event_manager_t::get().unsubscribe_rule(type, rule_binding);
}

void gaia::rules::unsubscribe_rules()
{
    return event_manager_t::get().unsubscribe_rules();
}
    
void gaia::rules::list_subscribed_rules(
    const char* ruleset_name, 
    const gaia_type_t* gaia_type, 
    const event_type_t* type, 
    list_subscriptions_t& subscriptions)
{
    event_manager_t::get().list_subscribed_rules(ruleset_name, gaia_type,
        type, subscriptions);
}
