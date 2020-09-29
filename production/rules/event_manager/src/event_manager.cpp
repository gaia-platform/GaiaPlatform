/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "retail_assert.hpp"
#include "event_manager.hpp"
#include "rule_stats_manager.hpp"
#include "gaia_db_internal.hpp"
#include "events.hpp"
#include "triggers.hpp"
#include "timer.hpp"

#include <cstring>
#include <variant>
#include <utility>

using namespace gaia::rules;
using namespace gaia::common;
using namespace gaia::db::triggers;
using namespace std;
using namespace std::chrono;

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
    // TODO[GAIAPLAT-111]: Check a configuration setting supplied by the
    // application developer for the number of threads to create.
    
    // Apply default settings.  See explanation in event_manager_settings.hpp.
    event_manager_settings_t settings;
    init(settings);
}

void event_manager_t::init(event_manager_settings_t& settings)
{
    m_invocations.reset(new rule_thread_pool_t(settings.num_background_threads));
    if (settings.enable_catalog_checks)
    {
        m_rule_checker.reset(new rule_checker_t());
    }

    // Currently rule stats collection and profiling the commit_trigger
    // function are enabled with one switch.
    rule_stats_manager_t::s_enabled = settings.enable_stats;
    m_timer.set_enabled(settings.enable_stats);

    auto fn = [](uint64_t transaction_id, const trigger_event_list_t& event_list) {
        event_manager_t::get().commit_trigger(transaction_id, event_list);
    };
    gaia::db::s_tx_commit_trigger = fn;

    m_is_initialized = true;
}


bool event_manager_t::process_last_operation_events(event_binding_t& binding, const trigger_event_t& event,
    steady_clock::time_point& start_time)
{
    bool rules_invoked = false;
    rule_list_t& rules = binding.last_operation_rules;

    for (auto rules_it = rules.begin(); rules_it != rules.end(); ++rules_it)
    {
        rules_invoked = true;
        enqueue_invocation(event, (*rules_it)->rule, start_time);
    }

    return rules_invoked;
}

bool event_manager_t::process_field_events(event_binding_t& binding,
    const trigger_event_t& event,
    steady_clock::time_point& start_time)
{
    if (binding.fields_map.size() == 0 || event.columns.size() == 0)
    {
        return false;
    }

    bool rules_invoked = false;

    for (field_position_t field_position : event.columns)
    {
        // Some rules refer to columns in this table.  Now see whether
        // the specific columns changed in this event are referenced
        // by any rules.  If not, keep going.
        auto field_it = binding.fields_map.find(field_position);
        if (field_it != binding.fields_map.end())
        {
            // If we got here then we have changed a field that is
            // referenced by at least one rule.  Schedule those rules
            // for invocation now.
            rule_list_t& rules = field_it->second;
            for (auto rules_it = rules.begin(); rules_it != rules.end(); ++rules_it)
            {
                rules_invoked = true;
                enqueue_invocation(event, (*rules_it)->rule, start_time);
            }
        }
    }

    return rules_invoked;
}

void event_manager_t::commit_trigger(uint64_t, const trigger_event_list_t& trigger_event_list)
{
    m_timer.log_function_duration([&]() {

    if (trigger_event_list.size() == 0)
    {
        return;
    }

    auto start_time = m_timer.get_time_point();

    // TODO[GAIAPLAT-308]: Event logging is only half the story. We
    // also need to do rule logging and the correlate the event instance
    // to the rules that it causes to fire.  This will then remove the
    // bool 'rule_invoked' flag.
    vector<bool> rules_invoked_list;

    for (size_t i = 0; i < trigger_event_list.size(); ++i)
    {
        const trigger_event_t& event = trigger_event_list[i];
        bool rules_invoked = false;

        auto type_it = m_subscriptions.find(event.container);
        if (type_it != m_subscriptions.end())
        {
            events_map_t& events = type_it->second;
            auto event_it = events.find(event.event_type);

            if (event_it != events.end())
            {
                // At least one rule is bound to this specific event.
                // The rule may be bound at the table level via the system
                // LastOperation field or at the column level by referencing
                // an active field in the rule body.
                event_binding_t& binding = event_it->second;

                // Once rules_invoked is true, we keep it there to mean
                // that any rule was subscribed to this event.
                if (process_last_operation_events(binding, event, start_time))
                {
                    rules_invoked = true;
                }

                if (process_field_events(binding, event, start_time))
                {
                    rules_invoked = true;
                }
            }
        }
        rules_invoked_list.emplace_back(rules_invoked);
    }

    // Enqueue a task to log all the events in this commit_trigger in a
    // separate thread in a new transaction.
    enqueue_invocation(trigger_event_list, rules_invoked_list, start_time);

    }, __PRETTY_FUNCTION__);
}

void event_manager_t::enqueue_invocation(const trigger_event_list_t& events,
    const vector<bool>& rules_invoked_list, 
    steady_clock::time_point& start_time)
{
    rule_thread_pool_t::log_events_invocation_t event_invocation {
        events, 
        rules_invoked_list
    };
    rule_thread_pool_t::invocation_t invocation {
        rule_thread_pool_t::invocation_type_t::log_events,
        std::move(event_invocation),
        rule_stats_manager_t::create_rule_stats(
            start_time, rule_stats_manager_t::c_log_event_tag)
    };
    m_invocations->enqueue(invocation);
} 

void event_manager_t::enqueue_invocation(const trigger_event_t& event, gaia_rule_fn rule_fn,
    steady_clock::time_point& start_time)
{
    rule_thread_pool_t::rule_invocation_t rule_invocation {
        rule_fn,
        event.container,
        event.event_type,
        event.record,
        event.columns
    };
    rule_thread_pool_t::invocation_t invocation{
        rule_thread_pool_t::invocation_type_t::rule,
        std::move(rule_invocation),
        rule_stats_manager_t::create_rule_stats(
            start_time, rule_stats_manager_t::c_rule_tag)
    };
    m_invocations->enqueue(invocation);
} 

void event_manager_t::check_subscription(
    event_type_t event_type,
    const field_position_list_t& fields)
{
    if (event_type == event_type_t::row_delete
        || event_type == event_type_t::row_insert)
    {
        if (fields.size() > 0)
        {
            throw invalid_subscription(event_type, "The field list must be empty.");
        }
    }
}

void event_manager_t::subscribe_rule(
    gaia_container_id_t container_id,
    event_type_t event_type,
    const field_position_list_t& fields,
    const rule_binding_t& rule_binding)
{
    // If any of these checks fail then an exception is thrown.
    check_rule_binding(rule_binding);
    check_subscription(event_type, fields);

    // Verify that the type and fields specified in the rule subscription
    // are valid according to the catalog.  The rule checker may be null
    // if the event_manager was initialized with 'disabled_catalog_checks'
    // set to true in its settings.
    if (m_rule_checker)
    {
        m_rule_checker->check_catalog(container_id, fields);
    }

    // Look up the container_id in our type map.  If we do not find it
    // then we create a new empty event map map.
    auto type_it = m_subscriptions.find(container_id);
    if (type_it == m_subscriptions.end()) 
    {
        auto inserted_type = m_subscriptions.insert(
            make_pair(container_id, events_map_t()));
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

    // We found or created an event entry. Now see if we have bound
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
                // TODO[GAIAPLAT-183]: Verify the field is in the catalog and
                // marked as an active field.
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
        // We are binding a table event to the LastOperation system field
        // because no field reference fields were provided.
        rule_list_t& rules = event_binding.last_operation_rules;
        add_rule(rules, rule_binding);
    }
}

bool event_manager_t::unsubscribe_rule(
    gaia_container_id_t container_id,
    event_type_t event_type, 
    const field_position_list_t& fields,
    const rule_binding_t& rule_binding)
{
    check_rule_binding(rule_binding);

    // If we haven't seen any subscriptions for this type
    // then no rule was bound.
    auto type_it = m_subscriptions.find(container_id);
    if (type_it == m_subscriptions.end())
    {
        return false;
    }

    // See if any rules are bound to the event.
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
    const gaia_container_id_t* container_id,
    const event_type_t* event_type_ptr,
    const uint16_t* field_ptr,
    subscription_list_t& subscriptions)
{
    subscriptions.clear();

    // Filter first by container_id, then event_type, then fields, then ruleset_name.
    for (auto type_it : m_subscriptions)
    {
        if (container_id && type_it.first != *container_id)
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
            // return any rules bound to the last operation.
            const rule_list_t& rules = event_binding.last_operation_rules;
            add_subscriptions(subscriptions, rules, type_it.first,
                event_it.first, 0, ruleset_name);
        }
    }
}

void event_manager_t::add_subscriptions(subscription_list_t& subscriptions, 
    const rule_list_t& rules,
    gaia_container_id_t container_id,
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

        subscriptions.emplace_back(make_unique<subscription_t>(
            rule->ruleset_name.c_str(),
            rule->rule_name.c_str(),
            container_id,
            event_type,
            field)
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

    // Do not allow the caller to bind the same rule to the same rule list.
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
    rules.emplace_back(this_rule);
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

// Enable conversion from rule_binding_t -> internal_rules_binding_t.
event_manager_t::_rule_binding_t::_rule_binding_t(
    const rule_binding_t& binding)
: _rule_binding_t(binding.ruleset_name, binding.rule_name, binding.rule)
{
}

event_manager_t::_rule_binding_t::_rule_binding_t(
    const char* a_ruleset_name, 
    const char* a_rule_name, 
    gaia_rule_fn a_rule)
{
    ruleset_name = a_ruleset_name;
    rule = a_rule;
    if (a_rule_name != nullptr) 
    {
        rule_name = a_rule_name;
    }
}



// Enable construction

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
    gaia_container_id_t container_id,
    event_type_t event_type,
    const field_position_list_t& fields,
    const rule_binding_t& rule_binding)
{
    event_manager_t::get().subscribe_rule(container_id, event_type, fields, rule_binding);
}

bool gaia::rules::unsubscribe_rule(
    gaia_container_id_t container_id,
    event_type_t event_type,
    const field_position_list_t& fields, 
    const gaia::rules::rule_binding_t& rule_binding)
{
    return event_manager_t::get().unsubscribe_rule(container_id, event_type, fields, rule_binding);
}

void gaia::rules::unsubscribe_rules()
{
    return event_manager_t::get().unsubscribe_rules();
}

void gaia::rules::list_subscribed_rules(
    const char* ruleset_name, 
    const gaia_container_id_t* container_id,
    const event_type_t* event_type,
    const uint16_t* field, 
    subscription_list_t& subscriptions)
{
    event_manager_t::get().list_subscribed_rules(ruleset_name, container_id,
        event_type, field, subscriptions);
}
