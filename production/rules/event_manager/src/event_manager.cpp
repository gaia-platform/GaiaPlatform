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
    // TODO: Check a configuration setting for the number of threads to create.
    // For now, execute in immediate mode and do not have multiple threads
    uint32_t num_threads = 0;
    m_invocations.reset(new rule_thread_pool_t(num_threads));
    m_is_initialized = true;
}

// TODO: the first parameter is the transaction id and it is currently unused.
void event_manager_t::commit_trigger(uint32_t, trigger_event_t* events, uint16_t num_events, bool immediate)
{
    for (uint16_t i = 0; i < num_events; i++)
    {
        const trigger_event_t* event = &events[i];
        bool rules_invoked = false;

        auto type_it = m_subscriptions.find(event->gaia_type);
        if (type_it != m_subscriptions.end())
        {
            // At least one rule is subscribed to events on this type.
            events_map_t& events = type_it->second;
            auto event_it = events.find(event->event_type);
            if (event_it != events.end())
            {
                // At least one rule is bound to this specific event.
                // The rule may be bound at the table level via the system
                // LastOperation field or at the column level by referencing
                // a trigger field in the rule body.
                event_binding_t& binding = event_it->second;

                // See if any rules are bound at the table level.  If so, schedule 
                // these rules to be invoked.
                rule_list_t& rules = binding.last_operation_rules;
                for (auto rules_it = rules.begin(); rules_it != rules.end(); ++rules_it)
                {
                    rules_invoked = true;
                    enqueue_invocation(event, *rules_it);
                }

                // See if any rules are bound to any columns that were 
                // changed as part of this event.  If so, then schedule these rules
                // to be invoked.
                if (binding.fields_map.size() > 0)
                {
                    for (uint16_t j = 0; j < event->num_column_ids; j++)
                    {
                        // Some rules refer to columns in this table.  Now see whether
                        // the specific columns changed in this event are referenced
                        // by any rules.  If not, keep going.
                        uint16_t col = event->column_ids[j];
                        auto field_it = binding.fields_map.find(col);
                        if (field_it != binding.fields_map.end())
                        {
                            // If we got here then we have changed a field that is
                            // referenced by at least one rule.  Schedule those rules
                            // for invocation now.
                            rule_list_t& rules = field_it->second;
                            for (auto rules_it = rules.begin(); rules_it != rules.end(); ++rules_it)
                            {
                                rules_invoked = true;
                                enqueue_invocation(event, *rules_it);
                            }
                        } 
                    }
                }
            }
        }

        // Log the event to our table regardless of whether any  rules will be invoked.
        log_to_db(event, rules_invoked);
    }

    if (immediate)
    {
        // execute any rules that were queued on the immediate queue
        m_invocations->execute_immediate();
    }
}

void event_manager_t::enqueue_invocation(const trigger_event_t* event, 
    const _rule_binding_t* binding)
{
    rule_context_t context({binding->ruleset_name.c_str(), binding->rule_name.c_str(), binding->rule}, 
        event->gaia_type, event->event_type, event->record_id);
    m_invocations->enqueue(context);
}

/*
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
*/

void event_manager_t::check_subscription(
    gaia_type_t gaia_type, 
    event_type_t event_type,
    const field_list_t& fields)
{
    if (is_transaction_event(event_type))
    {
        // TODO: use a constant for a non-zero gaia_type
        if (gaia_type != 0)
        {
            throw invalid_subscription(event_type, "The gaia_type must be zero.");
        }

        if (fields.size() > 0)
        {
            throw invalid_subscription(event_type, "The field list must be empty.");
        }
    }
    else
    {
        if (event_type == event_type_t::row_delete)
        {
            if (fields.size() > 0)
            {
                throw invalid_subscription(event_type, "The field list must be empty.");
            }
        }
    }
}

void event_manager_t::subscribe_rule(
    gaia_type_t gaia_type, 
    event_type_t event_type,
    const field_list_t& fields,
    const rule_binding_t& rule_binding)
{
    check_rule_binding(rule_binding);
    check_subscription(gaia_type, event_type, fields);

    // Look up the gaia_type in our type map.  If we do not find it
    // then we create a new empty event map map.
    auto type_it = m_subscriptions.find(gaia_type);
    if (type_it == m_subscriptions.end()) 
    {
        auto inserted_type = m_subscriptions.insert(
            make_pair(gaia_type, events_map_t()));
        type_it = inserted_type.first;
    }

    // We found or created a type entry.  Now see if we have an event entry.
    events_map_t& events = type_it->second;
    auto event_it = events.find(event_type);
    if (event_it == events.end())
    {
        auto inserted_event = events.insert(
            make_pair(event_type, event_binding_t()));
        event_it = inserted_event.first;
    }

    // We found or created an event entry.  Now see if we have bound
    // any events already.
    event_binding_t& event_binding = event_it->second;
    if (fields.size() > 0)
    {
        for (uint16_t field : fields)
        {
            fields_map_t& fields_map = event_binding.fields_map;
            auto field_it = fields_map.find(field);
            if (field_it == fields_map.end())
            {
                auto inserted_field = fields_map.insert(
                    make_pair(field, rule_list_t()));
                field_it = inserted_field.first;
            }

            rule_list_t& rules = field_it->second;
            add_rule(rules, rule_binding);
        }
    }
    else
    {
        // We are binding a table event here with no fields so bind the rule directly.
        rule_list_t& rules = event_binding.last_operation_rules;
        add_rule(rules, rule_binding);
    }
}

bool event_manager_t::unsubscribe_rule(
    gaia_type_t gaia_type,
    event_type_t event_type, 
    const field_list_t& fields,
    const rule_binding_t& rule_binding)
{
    check_rule_binding(rule_binding);

    // If we haven't seen any subscriptions for this type
    // then no rule was bound.
    auto type_it = m_subscriptions.find(gaia_type);
    if (type_it == m_subscriptions.end())
    {
        return false;
    }

    // See if any rules are bound to the event
    events_map_t& events = type_it->second;
    auto event_it = events.find(event_type);
    if (event_it == events.end())
    {
        return false;
    }

    bool removed_rule = false;
    event_binding_t& event_binding = event_it->second;
    if (fields.size() > 0)
    {
        // Remove this rule for all the fields that were subscribed
        // to it as described by the field_list.
        for (uint16_t field : fields)
        {
            auto field_it = event_binding.fields_map.find(field);
            if (field_it != event_binding.fields_map.end())
            {
                rule_list_t& rules = field_it->second;
                // If any rule was removed for any column then latch the return
                // value to true.
                if (remove_rule(rules, rule_binding))
                {
                    removed_rule = true;
                }
            }
        }
    }
    else
    {
        rule_list_t& rules = event_binding.last_operation_rules;
        removed_rule = remove_rule(rules, rule_binding);
    }

    return removed_rule;
}

void event_manager_t::unsubscribe_rules()
{
    m_subscriptions.clear();
    m_rules.clear();
}

void event_manager_t::list_subscribed_rules(
    const char* ruleset_name, 
    const gaia_type_t* gaia_type_ptr, 
    const event_type_t* event_type_ptr,
    const uint16_t* field_ptr,
    subscription_list_t& subscriptions)
{
    subscriptions.clear();

    // Filter first by gaia_type, then event_type, then fields, then ruleset_name.
    for (auto type_it : m_subscriptions)
    {
        if (gaia_type_ptr && type_it.first != *gaia_type_ptr)
        {
            continue;
        }

        const events_map_t& events = type_it.second;
        for (auto event_it : events)
        {
            if (event_type_ptr && event_it.first != *event_type_ptr)
            {
                continue;
            }

            const event_binding_t& event_binding = event_it.second;
            for (auto field : event_binding.fields_map)
            {
                if (field_ptr && field.first != *field_ptr)
                {
                    continue;
                }
                const rule_list_t& rules = field.second;
                add_subscriptions(subscriptions, rules, type_it.first, 
                    event_it.first, field.first, ruleset_name);
            }

            // If no field_ptr filter was passed in then also
            // return any rules bound to the last operation
            const rule_list_t& rules = event_binding.last_operation_rules;
            add_subscriptions(subscriptions, rules, type_it.first,
                event_it.first, 0, ruleset_name);
        }
    }
}

void event_manager_t::add_subscriptions(subscription_list_t& subscriptions, 
    const rule_list_t& rules,
    gaia_type_t gaia_type,
    event_type_t event_type,
    uint16_t field,
    const char* ruleset_filter)
{
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
            event_type,
            field}))
        );
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

void event_manager_t::log_to_db(const trigger_event_t* event, bool rules_invoked)
{
    static_assert(sizeof(uint32_t) == sizeof(event_type_t), 
        "event_type_t needs to be sizeof uint32_t");

    uint64_t timestamp = (uint64_t)time(NULL);

    // UNDONE:  when we support arrays in flatbuffers then we can pass in the 
    // column ordinals.  Until then, just pick out the first of the list.
    uint16_t column_id = 0;
    if (event->num_column_ids > 0)
    {
        column_id = event->column_ids[0];
    }

    {
        auto_tx_t tx;
        Event_log::insert_row((uint32_t)(event->event_type), (uint64_t)(event->gaia_type), 
            (uint64_t)(event->record_id), column_id, timestamp, rules_invoked);
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

void gaia::rules::subscribe_rule(
    gaia_type_t gaia_type,
    event_type_t event_type,
    const field_list_t& fields,
    const rule_binding_t& rule_binding)
{
    event_manager_t::get().subscribe_rule(gaia_type, event_type, fields, rule_binding);
}

bool gaia::rules::unsubscribe_rule(
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
    const uint16_t* field, 
    subscription_list_t& subscriptions)
{
    event_manager_t::get().list_subscribed_rules(ruleset_name, gaia_type,
        event_type, field, subscriptions);
}

void gaia::rules::commit_trigger(uint32_t tx_id, trigger_event_t* events, uint16_t num_events, bool immediate)
{
    event_manager_t::get().commit_trigger(tx_id, events, num_events, immediate);
}
