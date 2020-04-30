/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "retail_assert.hpp"
#include "event_manager.hpp"
#include "auto_tx.hpp"

#include <cstring>

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
    m_is_initialized = true;
}

bool event_manager_t::log_event(
    gaia_base_t* row, 
    event_type_t event_type, 
    event_mode_t mode)
{
    check_mode(mode);
    check_database_event(event_type, row);
    gaia_type_t gaia_type = row ? row->gaia_type() : 0;

    // Don't allow reentrant log_event calls.

    event_guard_t guard(m_database_event_guard, gaia_type, event_type);
    if (guard.is_blocked())
    {
        return false;
    }
    
    // Invoke all rules bound to this gaia_type and event_type immediately.
    auto type_it = m_database_subscriptions.find(gaia_type);
    if (type_it == m_database_subscriptions.end())
    {
        log_to_db(gaia_type, event_type, mode, row, false);
        return false;
    }

    events_map_t& events = type_it->second;
    rule_list_t& rules = events[event_type];
    bool rules_fired = rules.size() > 0;

    log_to_db(gaia_type, event_type, mode, row, rules_fired);

    for (auto rules_it = rules.begin(); rules_it != rules.end(); ++rules_it) 
    {
        const _rule_binding_t* rule_ptr = (*rules_it);
        rule_context_t context = create_rule_context(rule_ptr, gaia_type, event_type, row, nullptr);
        rule_ptr->rule(&context);
    }

    return rules_fired;
}

bool event_manager_t::log_event(
    gaia_base_t* row,
    const char* field,
    event_type_t event_type, 
    event_mode_t mode)
{
    check_mode(mode);
    check_field_event(event_type, row, field);
    gaia_type_t gaia_type = row ? row->gaia_type() : 0;

    // Don't allow reentrant log_event calls.
    event_guard_t guard(m_field_event_guard, gaia_type, field, event_type);
    if (guard.is_blocked())
    {
        return false;
    }
    
    // Invoke all rules bound to this gaia_type, column, and event_type immediately.
    auto type_it = m_field_subscriptions.find(gaia_type);
    if (type_it == m_field_subscriptions.end())
    {
        log_to_db(gaia_type, event_type, mode, row, field, false);
        return false;
    }

    fields_map_t& fields = type_it->second;
    auto field_it = fields.find(field);
    if (field_it == fields.end())
    {
        log_to_db(gaia_type, event_type, mode, row, field, false);
        return false;
    }

    events_map_t& events = field_it->second;
    rule_list_t& rules = events[event_type];
    bool rules_fired = rules.size() > 0;

    log_to_db(gaia_type, event_type, mode, row, field, rules_fired);

    for (auto rules_it = rules.begin(); rules_it != rules.end(); ++rules_it) 
    {
        const _rule_binding_t* rule_ptr = *rules_it;
        rule_context_t context = create_rule_context(rule_ptr, gaia_type, event_type, row, field);
        rule_ptr->rule(&context);
    }

    return rules_fired;
}

void event_manager_t::subscribe_rule(
    gaia_type_t gaia_type, 
    event_type_t event_type,
    const field_list_t& fields,
    const rule_binding_t& rule_binding)
{
    check_field_event(event_type);
    check_rule_binding(rule_binding);

    // Look up the gaia_type in our type map.  If we do not find it
    // then we create a new empty field map that will represent
    // the columns that have rules bound to them for this table
    // type.
    auto field_map_it = m_field_subscriptions.find(gaia_type);
    if (field_map_it == m_field_subscriptions.end()) 
    {
        auto inserted_field_map = m_field_subscriptions.insert(
            make_pair(gaia_type, fields_map_t()));
        field_map_it = inserted_field_map.first;
    }

    // Now we have a pointer to the fields map corresponding to the
    // gaia type.  Search this map for each field in the passed in 
    // field list.  If the field is not found in the field map then 
    // add it and its associated event map.
    fields_map_t& subscribed_fields = field_map_it->second;
    for (const string& field_name : fields)
    {
        auto event_map_it = subscribed_fields.find(field_name);
        if (event_map_it == subscribed_fields.end())
        {
            auto inserted_event_map = subscribed_fields.insert(
                make_pair(field_name, event_manager_t::create_field_event_map()));
            event_map_it = inserted_event_map.first;
        }

        // Finally add this rule for the appropriate event.
        events_map_t& events = event_map_it->second;
        rule_list_t& rules = events[event_type];
        add_rule(rules, rule_binding);
    }
}

void event_manager_t::subscribe_rule(
    gaia_type_t gaia_type, 
    event_type_t event_type, 
    const rule_binding_t& rule_binding)
{
    check_database_event(event_type);
    check_rule_binding(rule_binding);

    // If we've bound a rule to this gaia_type before then get
    // its event map. Otherwise, create the event map.
    auto event_map_it = m_database_subscriptions.find(gaia_type);
    if (event_map_it == m_database_subscriptions.end()) 
    {
        auto sub_it = m_database_subscriptions.insert(
            make_pair(gaia_type, event_manager_t::create_database_event_map()));
        event_map_it = sub_it.first;
    }

    // Retreive the rule_list associated with
    // the event_type we are binding to.
    events_map_t& events = event_map_it->second;
    rule_list_t& rules = events[event_type];
    add_rule(rules, rule_binding);
}

bool event_manager_t::unsubscribe_rule(
    gaia_type_t gaia_type, 
    event_type_t event_type, 
    const rule_binding_t& rule_binding)
{
    check_database_event(event_type);
    check_rule_binding(rule_binding);

    // If we haven't seen any subscriptions for this type
    // then no rule was bound.
    auto event_map_it = m_database_subscriptions.find(gaia_type);
    if (event_map_it == m_database_subscriptions.end())
    {
        return false;
    }

    // Get our rule list for the specific event specified
    // in the rule binding.
    events_map_t& events = event_map_it->second;
    rule_list_t& rules = events[event_type];
    return remove_rule(rules, rule_binding);;
}

bool event_manager_t::unsubscribe_rule(
    gaia_type_t gaia_type,
    event_type_t event_type, 
    const field_list_t& fields,
    const rule_binding_t& rule_binding)
{
    check_field_event(event_type);
    check_rule_binding(rule_binding);

    // If we haven't seen any subscriptions for this type
    // then no rule was bound.
    auto field_map_it = m_field_subscriptions.find(gaia_type);
    if (field_map_it == m_field_subscriptions.end())
    {
        return false;
    }

    // Remove this rule for all the fields that were subscribed
    // to it as described by the field_list
    bool removed_rule = false;

    fields_map_t& subscribed_fields = field_map_it->second;
    for (const string& field_name : fields)
    {
        auto event_map_it = subscribed_fields.find(field_name);
        if (event_map_it != subscribed_fields.end())
        {
            events_map_t& events = event_map_it->second;
            rule_list_t& rules = events[event_type];

            // If any rule was removed for any column then latch the return
            // value to true.
            if (remove_rule(rules, rule_binding))
            {
                removed_rule = true;
            }
        }
    }

    return removed_rule;
}

void event_manager_t::unsubscribe_rules()
{
    m_field_subscriptions.clear();
    m_database_subscriptions.clear();
    m_rules.clear();
}

void event_manager_t::list_subscribed_rules(
    const char* ruleset_name, 
    const gaia_type_t* gaia_type_ptr, 
    const event_type_t* type_ptr,
    const char* field_name,
    subscription_list_t& subscriptions)
{
    subscriptions.clear();

    // Filter first by gaia_type then field name, then event_type, then by 
    // ruleset_name.
    for (auto field_map_it : m_field_subscriptions)
    {
        if (gaia_type_ptr != nullptr && field_map_it.first != *gaia_type_ptr)
        {
            continue;
        }

        const fields_map_t& subscribed_fields = field_map_it.second;
        for (auto field : subscribed_fields)
        {
            if (field_name 
                && (0 != strcmp(field_name, field.first.c_str())))
            {
                continue;
            }

            auto events = field.second;
            add_subscriptions(subscriptions, events, field_map_it.first, 
                field.first.c_str(), ruleset_name, type_ptr);
        }
    }
        
    // Filter first by gaia_type, then event_type, then by ruleset_name.
    for (auto event_map_it : m_database_subscriptions)
    {
        if (gaia_type_ptr != nullptr && event_map_it.first != *gaia_type_ptr)
        {
            continue;
        }

        auto events = event_map_it.second;
        add_subscriptions(subscriptions, events, event_map_it.first, nullptr, 
            ruleset_name, type_ptr);
    }
}

void event_manager_t::add_subscriptions(subscription_list_t& subscriptions, 
    const events_map_t& events, 
    gaia_type_t gaia_type,
    const char * field_name,
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
                gaia_type, 
                event_it.first,
                field_name}))
            );
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

event_manager_t::events_map_t event_manager_t::create_database_event_map() {
    return {
        make_pair(event_type_t::transaction_begin, list<const _rule_binding_t*>()),
        make_pair(event_type_t::transaction_commit, list<const _rule_binding_t*>()),
        make_pair(event_type_t::transaction_rollback, list<const _rule_binding_t*>()),
        make_pair(event_type_t::row_insert, list<const _rule_binding_t*>()),
        make_pair(event_type_t::row_update, list<const _rule_binding_t*>()),
        make_pair(event_type_t::row_delete, list<const _rule_binding_t*>())
    };
}

event_manager_t::events_map_t event_manager_t::create_field_event_map() {
    return {
        make_pair(event_type_t::field_read, list<const _rule_binding_t*>()),
        make_pair(event_type_t::field_write, list<const _rule_binding_t*>())
    };
}

void event_manager_t::log_to_db(gaia_type_t gaia_type, 
    event_type_t event_type, 
    event_mode_t event_mode,
    gaia_base_t* context,
    const char * field,
    bool rules_fired)
{
    static_assert(sizeof(uint32_t) == sizeof(event_type_t), 
        "event_type_t needs to be sizeof uint32_t");
    static_assert(sizeof(uint8_t) == sizeof(event_mode_t), 
        "event_mode_t needs to be sizeof uint8_t");

    uint64_t timestamp = (uint64_t)time(NULL);
    string event_source;
    gaia_id_t context_id = 0;

    // The event source is either the field name for field events
    // or the name of the Gaia table type for database events.
    // In case there is no context supplied (begin/commit/rollback)
    // events, for example, there is no source or context id.
    if (context)
    {
        event_source = context->gaia_typename();
        if (field)
        {
            event_source.append(".");
            event_source.append(field);
        }
        context_id = context->gaia_id();
    }

    {
        auto_tx_t tx;
        Event_log::insert_row((uint64_t)gaia_type, (uint32_t)event_type, 
            (uint8_t) event_mode, event_source.c_str(), timestamp, context_id, rules_fired);
        tx.commit();
    }
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

// Construct rule_context_t here 
gaia::rules::rule_context_t::rule_context_t(
    const rule_binding_t& a_binding, 
    gaia::common::gaia_type_t a_gaia_type,
    event_type_t a_event_type,
    gaia::common::gaia_base_t * a_row,
    const char* a_field
)
    : rule_binding(a_binding)
    , gaia_type(a_gaia_type)
    , event_type(a_event_type)
    , event_context(a_row)
{
    // The event source is either the column name (for a field event)
    // or the type name (for a database event).  If the database event
    // is not scoped to a type (begin/commit/rollback transaction events)
    // then the source field is emtpy.
    if (a_field)
    {
        event_source = a_row->gaia_typename();
        event_source.append(".");
        event_source.append(a_field);
    }
    else
    if (a_row)
    {
        event_source = a_row->gaia_typename();
    }
}

/**
 * Public event API implementation
 */
bool gaia::rules::log_database_event(
    gaia_base_t* row, 
    event_type_t type, 
    event_mode_t mode)
{
    return event_manager_t::get().log_event(row, type, mode);
}

bool gaia::rules::log_field_event(
    gaia_base_t* row,
    const char* field,
    event_type_t type,
    event_mode_t mode)
{
    return event_manager_t::get().log_event(row, field, type, mode);
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

void gaia::rules::subscribe_database_rule(
    gaia_type_t gaia_type, 
    event_type_t type, 
    const rule_binding_t& rule_binding)
{
    event_manager_t::get().subscribe_rule(gaia_type, type, rule_binding);
}

void gaia::rules::subscribe_field_rule(
    gaia_type_t gaia_type,
    event_type_t event_type,
    const field_list_t& fields,
    const rule_binding_t& rule_binding)
{
    event_manager_t::get().subscribe_rule(gaia_type, event_type, fields, rule_binding);
}

bool gaia::rules::unsubscribe_database_rule(
    gaia_type_t gaia_type, 
    event_type_t type, 
    const gaia::rules::rule_binding_t& rule_binding)
{
    return event_manager_t::get().unsubscribe_rule(gaia_type, type, rule_binding);
}

bool gaia::rules::unsubscribe_field_rule(
    gaia_type_t gaia_type, 
    event_type_t event_type,
    const field_list_t& fields, 
    const gaia::rules::rule_binding_t& rule_binding)
{
    return event_manager_t::get().unsubscribe_rule(gaia_type, event_type, fields, rule_binding);
}

void gaia::rules::unsubscribe_rules()
{
    return event_manager_t::get().unsubscribe_rules();
}

void gaia::rules::list_subscribed_rules(
    const char* ruleset_name, 
    const gaia_type_t* gaia_type, 
    const event_type_t* event_type,
    const char* field_name, 
    subscription_list_t& subscriptions)
{
    event_manager_t::get().list_subscribed_rules(ruleset_name, gaia_type,
        event_type, field_name, subscriptions);
}
