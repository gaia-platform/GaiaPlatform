/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "event_manager.hpp"

using namespace gaia::events;
using namespace gaia::rules;
using namespace gaia::common;
using namespace std;

#include <cstring>

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

gaia::common::error_code_t event_manager_t::log_event(event_type type, event_mode mode)
{
    // todo: log the event in a table

    if (event_mode::deferred == mode) {
        return error_code_t::event_mode_not_supported;
    }

    // invoke all rules immediately
    rule_list_t& rules = m_transaction_subscriptions[type];
    for (auto rules_it = rules.begin(); rules_it != rules.end(); rules_it++) {
            const _rule_binding_t * rule_ptr = *rules_it;
            transaction_context_t tc(
                {rule_ptr->ruleset_name.c_str(), rule_ptr->rule_name.c_str(), rule_ptr->rule},
                type);

            rule_ptr->rule(&tc);
    }

    return error_code_t::success;
}

gaia::common::error_code_t event_manager_t::log_event(
    gaia_base * row, 
    gaia_type_t gaia_type,
    event_type type, 
    event_mode mode)
{
    // todo: log the event in a table

    if (event_mode::deferred == mode) {
        return error_code_t::event_mode_not_supported;
    }

    // invoke all rules immediately
    auto type_it = m_table_subscriptions.find(gaia_type);
    if (type_it == m_table_subscriptions.end()) {
        return error_code_t::success;
    }
   
    events_map_t& events = type_it->second;
    rule_list_t& rules = events[type];

    for (auto rules_it = rules.begin(); rules_it != rules.end(); rules_it++) {
            const _rule_binding_t * rule_ptr = *rules_it;
            table_context_t tc(
                {rule_ptr->ruleset_name.c_str(), rule_ptr->rule_name.c_str(), rule_ptr->rule},
                type, 
                gaia_type, 
                row);
            rule_ptr->rule(&tc);
    }

    return error_code_t::success;
}

error_code_t event_manager_t::subscribe_rule(
    gaia_type_t gaia_type, 
    event_type type, 
    const rule_binding_t& rule_binding)
{
    // we only support table events scoped to a gaia_type for now
    if (!is_valid_table_event(type)) {
        return error_code_t::invalid_event_type;
    }

    if (!is_valid_rule_binding(rule_binding)) {
        return error_code_t::invalid_rule_binding;        
    }

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
    rule_list_t& rules = events[type];

    return add_rule(rules, rule_binding);
}

error_code_t event_manager_t::subscribe_rule(
    event_type type, 
    const rule_binding_t& rule_binding)
{
    if (!is_valid_transaction_event(type)) {
        return error_code_t::invalid_event_type;
    }

    if (!is_valid_rule_binding(rule_binding)) {
        return error_code_t::invalid_rule_binding;        
    }

    // If we've haven't bound any rules yet then the events
    // map will be empty.  Lazily create it
    // its event map. Otherwise, create the event map
    if (0 == m_transaction_subscriptions.size()) {
        insert_transaction_events(m_transaction_subscriptions);
    }

    // Retreive the rule_list associated with
    // the event_type we are binding to.
    rule_list_t& rules = m_transaction_subscriptions[type];

    return add_rule(rules, rule_binding);
}

gaia::common::error_code_t event_manager_t::unsubscribe_rule(
    gaia_type_t gaia_type, 
    event_type type, 
    const rule_binding_t& rule_binding)
{
    // We only support table events scoped to a gaia_type for now.
    if (!is_valid_table_event(type)) {    
        return error_code_t::invalid_event_type;
    }

    // Since this rule must have been seen before to unsubscribe it
    // its members should be fully populated.
    if (!is_valid_rule_binding(rule_binding)) {
        return error_code_t::invalid_rule_binding;
    }

    // If we haven't seen any subscriptions for this type
    // then no rule was bound.
    auto type_it = m_table_subscriptions.find(gaia_type);
    if (type_it == m_table_subscriptions.end()) {
        return error_code_t::rule_not_found;
    }

    // Get our rule list for the specific event specified
    // in the rule binding.
    events_map_t& events = type_it->second;
    rule_list_t& rules = events[type];

    return remove_rule(rules, rule_binding);
}

error_code_t event_manager_t::unsubscribe_rule(
    event_type type, 
    const gaia::rules::rule_binding_t& rule_binding)
{
    // We only support table events scoped to a gaia_type for now.
    if (!is_valid_transaction_event(type)) {    
        return error_code_t::invalid_event_type;
    }

    // Since this rule must have been seen before to unsubscribe it
    // its members should be fully populated.
    if (!is_valid_rule_binding(rule_binding)) {
        return error_code_t::invalid_rule_binding;
    }

    // Get our rule list for the specific event specified
    // in the rule binding.
    rule_list_t& rules = m_transaction_subscriptions[type];

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
    const event_type* type_ptr,
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
    const char * ruleset_filter, 
    const event_type * event_filter)
{
    for (auto event_it : events)
    {
        if (event_filter != nullptr 
            && event_it.first != *event_filter) 
        {
            continue;
        }

        const rule_list_t& rules = event_it.second;
        for (auto rule : rules) 
        {
            if (nullptr != ruleset_filter
                && (0 != strcmp(ruleset_filter, rule->ruleset_name.c_str())))
            {
                continue;
            }

            subscriptions.push_back(unique_ptr<subscription_t>(new subscription_t({
                rule->ruleset_name.c_str(),
                rule->ruleset_name.c_str(),
                gaia_type, event_it.first})));
        }
    }
}

bool event_manager_t::is_valid_table_event(event_type type) 
{
    return (type == event_type::col_change 
            || type == event_type::row_delete 
            || type == event_type::row_insert 
            || type == event_type::row_update);
}

bool event_manager_t::is_valid_transaction_event(event_type type) 
{
    return (type == event_type::transaction_begin 
            || type == event_type::transaction_commit 
            || type == event_type::transaction_rollback);
}

bool event_manager_t::is_valid_rule_binding(
    const rule_binding_t& binding)
{
    return (nullptr != binding.ruleset_name
            && nullptr != binding.rule_name
            && nullptr != binding.rule);
}

gaia::common::error_code_t event_manager_t::add_rule(
    rule_list_t& rules,
    const rule_binding_t& binding)
{
    // Don't allow adding a rule that has the same
    // key as another rule but is bound to a different
    // rule function
    const _rule_binding_t * rule_ptr = find_rule(binding);
    if (rule_ptr != nullptr && rule_ptr->rule != binding.rule) {
        return error_code_t::duplicate_rule;
    }

    // Also dont' allow the caller to bind the same
    // rule to the same rule list.  This is most likely
    // a programming error.
    for (auto rules_it = rules.begin(); rules_it != rules.end(); rules_it++){
        if (*rules_it == rule_ptr) {
            return error_code_t::duplicate_rule;
        }
    }

    // If we already anave seen this rule, then
    // add it to the list.  Otherwise, create a new 
    // rule binding entry and put it in our global list.
    _rule_binding_t * this_rule = nullptr;
    if (rule_ptr == nullptr) {
        this_rule = new _rule_binding_t(binding);
         m_rules.insert(make_pair(make_rule_key(binding), 
            unique_ptr<_rule_binding_t>(this_rule)));
    }
    else {
        this_rule = const_cast<_rule_binding_t *>(rule_ptr);
    }

    // finally add the rule to the subscription list.
    rules.push_back(this_rule);

    return error_code_t::success;
}

gaia::common::error_code_t event_manager_t::remove_rule(
    rule_list_t& rules,
    const rule_binding_t& binding)
{
    const _rule_binding_t * rule_ptr = find_rule(binding);
    if (rule_ptr == nullptr) {
        return error_code_t::rule_not_found;
    }

    // Remove the rule based on the pointer to the rule
    // binding.  If we didn't find this rule for removal
    // then return an error.
    auto size = rules.size();
    rules.remove_if([&] (const _rule_binding_t * ptr){
        return (ptr == rule_ptr);
    });
    if (size == rules.size()) {
        return error_code_t::rule_not_found;
    }

    return error_code_t::success;
}

const event_manager_t::_rule_binding_t * event_manager_t::find_rule(const rule_binding_t& binding)
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
        make_pair(event_type::col_change, list<const _rule_binding_t*>()),
        make_pair(event_type::row_insert, list<const _rule_binding_t*>()),
        make_pair(event_type::row_update, list<const _rule_binding_t*>()),
        make_pair(event_type::row_delete, list<const _rule_binding_t*>())
    };
}

void event_manager_t::insert_transaction_events(event_manager_t::events_map_t& transaction_map) {
    transaction_map.insert(make_pair(event_type::transaction_begin, list<const _rule_binding_t*>()));
    transaction_map.insert(make_pair(event_type::transaction_commit, list<const _rule_binding_t*>()));
    transaction_map.insert(make_pair(event_type::transaction_rollback, list<const _rule_binding_t*>()));
}

// allow conversion from rule_binding_t -> internal_rules_binding_t
event_manager_t::_rule_binding_t::_rule_binding_t(
    const rule_binding_t& binding)
{
    ruleset_name = binding.ruleset_name;
    rule = binding.rule;
    if (binding.rule_name != nullptr) {
        rule_name = binding.rule_name;
    }
}

/**
 * Public event API implementation
 */
gaia::common::error_code_t gaia::events::log_transaction_event(event_type type, event_mode mode)
{
    return event_manager_t::get().log_event(type, mode);
}

error_code_t gaia::events::log_table_event(
    gaia_base * row, 
    gaia_type_t gaia_type,
    event_type type, 
    event_mode mode)
{
    return event_manager_t::get().log_event(row, gaia_type, type, mode);
}

/**
 * Public rules API implementation
 */
error_code_t gaia::rules::subscribe_table_rule(
    gaia_type_t gaia_type, 
    event_type type, 
    const rule_binding_t& rule_binding)
{
    return event_manager_t::get().subscribe_rule(gaia_type, type, rule_binding);
}

error_code_t gaia::rules::subscribe_transaction_rule(
    event_type type, 
    const gaia::rules::rule_binding_t& rule_binding)
{
    return event_manager_t::get().subscribe_rule(type, rule_binding);
}

gaia::common::error_code_t gaia::rules::unsubscribe_table_rule(
    gaia_type_t gaia_type, 
    event_type type, 
    const gaia::rules::rule_binding_t& rule_binding)
{
    return event_manager_t::get().unsubscribe_rule(gaia_type, type, rule_binding);
}

error_code_t gaia::rules::unsubscribe_transaction_rule(
    event_type type, 
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
    const event_type* type, 
    list_subscriptions_t& subscriptions)
{
    event_manager_t::get().list_subscribed_rules(ruleset_name, gaia_type,
        type, subscriptions);
}
