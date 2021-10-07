/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "event_manager.hpp"

#include <cstring>

#include <utility>
#include <variant>

#include "gaia/events.hpp"

#include "gaia_internal/common/logger_internal.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/timer.hpp"
#include "gaia_internal/db/gaia_db_internal.hpp"
#include "gaia_internal/db/triggers.hpp"

#include "rule_stats_manager.hpp"

using namespace gaia::rules;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace std;
using namespace std::chrono;

/**
 * These functions are supplied by the user (either via the translation engine or custom application code).
 * However, since they are optional, provide implementations to satisfy the linker here.
 * If the user does provide their own implementation, the linker will choose the strong reference.
 */
extern "C" void __attribute__((weak)) initialize_rules()
{
}

extern "C" void __attribute__((weak)) handle_rule_exception()
{
}

/**
 * Class implementation
 */
event_manager_t& event_manager_t::get(bool require_initialized)
{
    static event_manager_t s_instance;

    // Throw an error if the instance must be initialized before retrieving the instance.
    if (require_initialized && !s_instance.m_is_initialized)
    {
        throw initialization_error();
    }

    return s_instance;
}

void event_manager_t::init()
{
    // Apply default settings.  See explanation in event_manager_settings.hpp.
    event_manager_settings_t settings;
    init(settings);
}

void event_manager_t::init(const event_manager_settings_t& settings)
{
    unique_lock<recursive_mutex> lock(m_init_lock);

    if (m_is_initialized)
    {
        shutdown();
    }

    gaia_log::rules().debug("Initializing rules engine...");

    size_t count_worker_threads = settings.num_background_threads;
    if (count_worker_threads == SIZE_MAX)
    {
        count_worker_threads = thread::hardware_concurrency();
    }

    m_stats_manager = make_unique<rule_stats_manager_t>(
        settings.enable_rule_stats,
        count_worker_threads,
        settings.stats_log_interval);

    m_rule_checker = make_unique<rule_checker_t>(
        settings.enable_catalog_checks, settings.enable_db_checks);

    m_invocations = make_unique<rule_thread_pool_t>(
        count_worker_threads, settings.max_rule_retries, *m_stats_manager, *m_rule_checker);

    m_trigger_fn = [](const trigger_event_list_t& event_list) {
        event_manager_t::get().commit_trigger(event_list);
    };
    set_commit_trigger(m_trigger_fn);

    m_is_initialized = true;
}

void event_manager_t::shutdown()
{
    unique_lock<recursive_mutex> lock(m_init_lock);

    if (!m_is_initialized)
    {
        return;
    }

    // Destroy the thread pool first to ensure that any scheduled rules get a chance to execute.
    // Do not reset the m_invocations pointer yet because executing rules may cause other rules to
    // be scheduled.
    m_invocations->shutdown();

    // Now that all the scheduled rules have been invoked, don't allow any new rules to be scheduled.
    set_commit_trigger(nullptr);

    m_invocations.reset();
    m_stats_manager.reset();
    m_rule_checker.reset();

    // Don't uninitialize until we've shutdown the thread pool.
    m_is_initialized = false;

    // Ensure we can re-initialize by dropping our subscription state.
    unsubscribe_rules();
}

void event_manager_t::process_last_operation_events(
    event_binding_t& binding, const trigger_event_t& event,
    steady_clock::time_point& start_time)
{
    rule_list_t& rules = binding.last_operation_rules;

    for (auto const& rule_binding : rules)
    {
        gaia_log::rules().trace("Enqueue table event:'{}', txn_id:'{}', gaia_type:'{}', gaia_id:'{}'", event_type_name(event.event_type), event.txn_id, event.gaia_type, event.record);
        enqueue_invocation(event, rule_binding, start_time, binding.serial_group);
    }
}

void event_manager_t::process_field_events(
    event_binding_t& binding, const trigger_event_t& event,
    steady_clock::time_point& start_time)
{
    if (binding.fields_map.size() == 0 || event.columns.size() == 0)
    {
        return;
    }

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
            for (auto const& rule_binding : rules)
            {
                gaia_log::rules().trace(
                    "Enqueue field event:'{}', txn_id:'{}', gaia_type:'{}', field_id:'{}', gaia_id:'{}'",
                    event_type_name(event.event_type), event.txn_id, event.gaia_type, field_position, event.record);

                enqueue_invocation(event, rule_binding, start_time, binding.serial_group);
            }
        }
    }
}

void event_manager_t::commit_trigger(const trigger_event_list_t& trigger_event_list)
{
    if (trigger_event_list.size() == 0)
    {
        return;
    }

    auto start_time = gaia::common::timer_t::get_time_point();

    // TODO[GAIAPLAT-308]: Event logging is only half the story. We
    // also need to do rule logging and the correlate the event instance
    // to the rules that it causes to fire. We also do not support trimming
    // tables yet so we are not going to insert rows into the event log here.

    for (const auto& event : trigger_event_list)
    {
        // TODO logging this as db() and not as rules() for 2 reasons:
        //   1. This should be logged in the DB but we have no logging there yet.
        //   2. I don't want to pollute the rules() output too much.
        // When DB logging is available move this statement there.
        gaia_log::db().trace(
            "commit_trigger:'{}' txn_id:'{}', gaia_type:'{}', gaia_id:'{}'",
            event_type_name(event.event_type), event.txn_id, event.gaia_type, event.record);

        auto type_it = m_subscriptions.find(event.gaia_type);
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

                process_last_operation_events(binding, event, start_time);
                process_field_events(binding, event, start_time);
            }
        }
    }
}

void event_manager_t::enqueue_invocation(
    const trigger_event_t& event,
    const _rule_binding_t* rule_binding,
    steady_clock::time_point& start_time,
    shared_ptr<rule_thread_pool_t::serial_group_t>& serial_group)
{
    rule_thread_pool_t::rule_invocation_t rule_invocation{
        rule_binding->rule,
        event.gaia_type,
        event.event_type,
        event.record,
        event.txn_id};
    rule_thread_pool_t::invocation_t invocation{
        std::move(rule_invocation),
        rule_binding->log_rule_name.c_str(),
        start_time,
        serial_group};
    m_invocations->enqueue(invocation);
}

void event_manager_t::check_subscription(event_type_t event_type, const field_position_list_t& fields)
{
    //TODO[GAIAPLAT-445] We don't expose deleted row events
    //if (event_type == event_t::row_delete || event_type == event_type_t::row_insert)
    if (event_type == event_type_t::row_insert)
    {
        if (fields.size() > 0)
        {
            throw invalid_subscription(event_type, "The field list must be empty.");
        }
    }
}

void event_manager_t::subscribe_rule(
    gaia_type_t gaia_type,
    event_type_t event_type,
    const field_position_list_t& fields,
    const rule_binding_t& rule_binding)
{
    // If any of these checks fail then an exception is thrown.
    check_rule_binding(rule_binding);
    check_subscription(event_type, fields);

    // Verify that the type and fields specified in the rule subscription
    // are valid according to the catalog.
    m_rule_checker->check_catalog(gaia_type, fields);

    // Look up the gaia_type in our type map.  If we do not find it
    // then we create a new empty event map map.
    auto type_it = m_subscriptions.find(gaia_type);
    if (type_it == m_subscriptions.end())
    {
        auto inserted_type = m_subscriptions.insert(make_pair(gaia_type, events_map_t()));
        type_it = inserted_type.first;
    }

    // We found or created a type entry.  Now see if we have an event entry.
    events_map_t& events = type_it->second;
    auto event_it = events.find(event_type);
    if (event_it == events.end())
    {
        auto inserted_event = events.insert(make_pair(event_type, event_binding_t()));
        event_it = inserted_event.first;
    }

    // We found or created an event entry. Now see if we have bound
    // any events already.
    event_binding_t& event_binding = event_it->second;
    bool is_rule_subscribed = false;
    if (fields.size() > 0)
    {
        for (uint16_t field : fields)
        {
            fields_map_t& fields_map = event_binding.fields_map;
            auto field_it = fields_map.find(field);
            if (field_it == fields_map.end())
            {
                auto inserted_field = fields_map.insert(make_pair(field, rule_list_t()));
                field_it = inserted_field.first;
            }

            rule_list_t& rules = field_it->second;
            is_rule_subscribed = add_rule(rules, rule_binding);
        }
    }
    else
    {
        // We are binding a table event to the LastOperation system field
        // because no field reference fields were provided.
        rule_list_t& rules = event_binding.last_operation_rules;
        is_rule_subscribed = add_rule(rules, rule_binding);
    }

    if (rule_binding.serial_group_name != nullptr && event_binding.serial_group == nullptr)
    {
        event_binding.serial_group = m_serial_group_manager.acquire_group(rule_binding.serial_group_name);
    }

    if (is_rule_subscribed)
    {
        gaia_log::rules().debug("Rule '{}:{}:{}' successfully subscribed.", rule_binding.ruleset_name, rule_binding.rule_name, rule_binding.line_number);
    }
}

bool event_manager_t::unsubscribe_rule(
    gaia_type_t gaia_type,
    event_type_t event_type,
    const field_position_list_t& fields, const rule_binding_t& rule_binding)
{
    check_rule_binding(rule_binding);

    // If we haven't seen any subscriptions for this type
    // then no rule was bound.
    auto type_it = m_subscriptions.find(gaia_type);
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

    bool is_rule_removed = false;
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
                    is_rule_removed = true;
                }
            }
        }
    }
    else
    {
        rule_list_t& rules = event_binding.last_operation_rules;
        is_rule_removed = remove_rule(rules, rule_binding);
    }

    if (is_rule_removed)
    {
        gaia_log::rules().debug("Rule '{}:{}:{}' successfully unsubscribed.", rule_binding.ruleset_name, rule_binding.rule_name, rule_binding.line_number);
    }

    return is_rule_removed;
}

void event_manager_t::unsubscribe_rules()
{
    gaia_log::rules().debug("Unsubscribing all rules.");

    // Because a rule may cause further invocations on commit which cause access of the
    // rule subscriptions, we need to wait for the current rules graph to finish executing.
    if (m_invocations)
    {
        m_invocations->wait_for_rules_to_complete();

        // Detach the commit trigger so that any new events that come in do not try
        // to look for rule subscriptions while we are removing them.
        gaia::db::set_commit_trigger(nullptr);

        // Since an invocation might have snuck in between the time we finished executing
        // and the time we detached the commit trigger, wait for these to finish.
        m_invocations->wait_for_rules_to_complete();
    }

    // Now it is safe to clear out the subscriptions and rule bindings.
    m_subscriptions.clear();
    m_rules.clear();

    // Reset the commit trigger so we are ready to go on any new
    // rule subscriptions.
    gaia::db::set_commit_trigger(m_trigger_fn);
}

void event_manager_t::list_subscribed_rules(
    const char* ruleset_name,
    const gaia_type_t* gaia_type_ptr,
    const event_type_t* event_type_ptr,
    const field_position_t* field_ptr,
    subscription_list_t& subscriptions)
{
    subscriptions.clear();

    // Filter first by gaia_type, then event_type, then fields, then ruleset_name.
    for (auto const& type_it : m_subscriptions)
    {
        if (gaia_type_ptr && type_it.first != *gaia_type_ptr)
        {
            continue;
        }

        const events_map_t& events = type_it.second;
        for (auto const& event_it : events)
        {
            if (event_type_ptr && event_it.first != *event_type_ptr)
            {
                continue;
            }

            const event_binding_t& event_binding = event_it.second;
            for (auto const& field : event_binding.fields_map)
            {
                if (field_ptr && field.first != *field_ptr)
                {
                    continue;
                }
                const rule_list_t& rules = field.second;
                add_subscriptions(subscriptions, rules, type_it.first, event_it.first, field.first, ruleset_name);
            }

            // If no field_ptr filter was passed in then also
            // return any rules bound to the last operation.
            const rule_list_t& rules = event_binding.last_operation_rules;
            add_subscriptions(subscriptions, rules, type_it.first, event_it.first, 0, ruleset_name);
        }
    }
}

void event_manager_t::add_subscriptions(
    subscription_list_t& subscriptions,
    const rule_list_t& rules,
    gaia_type_t gaia_type,
    event_type_t event_type,
    field_position_t field,
    const char* ruleset_filter)
{
    for (auto rule : rules)
    {
        if (ruleset_filter && (0 != strcmp(ruleset_filter, rule->ruleset_name.c_str())))
        {
            continue;
        }

        subscriptions.emplace_back(make_unique<subscription_t>(
            rule->ruleset_name.c_str(),
            rule->rule_name.c_str(),
            gaia_type, event_type,
            field,
            rule->line_number,
            rule->serial_group_name.c_str()));
    }
}

bool event_manager_t::add_rule(rule_list_t& rules, const rule_binding_t& binding)
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
    for (auto const& rule : rules)
    {
        if (rule == rule_ptr)
        {
            throw duplicate_rule(binding, false);
        }
    }

    // If we already have seen this rule, then
    // add it to the list.  Otherwise, create a new
    // rule binding entry and put it in our global list.
    if (rule_ptr == nullptr)
    {
        const string& key = _rule_binding_t::make_key(binding.ruleset_name, binding.rule_name);
        auto rule_binding = new _rule_binding_t(binding);
        m_rules.insert(make_pair(key, unique_ptr<_rule_binding_t>(rule_binding)));
        rules.emplace_back(rule_binding);
    }
    else
    {
        rules.emplace_back(rule_ptr);
    }

    return true;
}

bool event_manager_t::remove_rule(rule_list_t& rules, const rule_binding_t& binding)
{
    bool is_rule_removed = false;
    const _rule_binding_t* rule_ptr = find_rule(binding);

    if (rule_ptr)
    {
        auto size = rules.size();
        rules.remove_if([&](const _rule_binding_t* ptr) { return (ptr == rule_ptr); });
        is_rule_removed = (size != rules.size());
    }

    return is_rule_removed;
}

const event_manager_t::_rule_binding_t* event_manager_t::find_rule(const rule_binding_t& binding)
{
    auto rule_it = m_rules.find(_rule_binding_t::make_key(binding.ruleset_name, binding.rule_name));
    if (rule_it != m_rules.end())
    {
        return rule_it->second.get();
    }

    return nullptr;
}

string event_manager_t::_rule_binding_t::make_key(const char* ruleset_name, const char* rule_name)
{
    string key = ruleset_name;
    key.append(rule_name);
    return key;
}

// Enable conversion from rule_binding_t -> internal_rules_binding_t.
event_manager_t::_rule_binding_t::_rule_binding_t(const rule_binding_t& binding)
    : _rule_binding_t(
        binding.ruleset_name,
        binding.rule_name,
        binding.rule,
        binding.line_number,
        binding.serial_group_name)
{
}

event_manager_t::_rule_binding_t::_rule_binding_t(
    const char* ruleset_name,
    const char* rule_name,
    gaia_rule_fn rule,
    uint32_t line_number,
    const char* serial_group_name)
{
    this->ruleset_name = ruleset_name;
    this->rule = rule;
    this->line_number = line_number;

    // Create a log/trace friendly name.
    // [<rule line number>] <ruleset_name>::<rule_name>
    log_rule_name = "[";
    log_rule_name.append(to_string(line_number));
    log_rule_name.append("] ");
    log_rule_name.append(ruleset_name);

    if (rule_name != nullptr)
    {
        this->rule_name = rule_name;
        log_rule_name.append("::");
        log_rule_name.append(rule_name);
    }
    if (serial_group_name != nullptr)
    {
        this->serial_group_name = serial_group_name;
    }
}

// Initialize the rules engine with settings from a user-supplied gaia configuration file.
void gaia::rules::initialize_rules_engine(shared_ptr<cpptoml::table>& root_config)
{
    bool require_initialized = false;
    // Create default settings for the rules engine and then override them with
    // user-supplied configuration values;
    event_manager_settings_t settings;

    // Override the default settings with any configuration settings;
    event_manager_settings_t::parse_rules_config(root_config, settings);
    event_manager_t::get(require_initialized).init(settings);
    initialize_rules();
}

/**
 * Public rules API implementation
 */
void gaia::rules::initialize_rules_engine()
{
    bool require_initialized = false;
    event_manager_t::get(require_initialized).init();

    /**
     * This function must be provided by the
     * rules application.  This function is
     * generated by the gaia preprocessor on
     * behalf of the user.
     */
    initialize_rules();
}

void gaia::rules::shutdown_rules_engine()
{
    // Allow shutdown to be called even if the rules engine has not been initialized.
    bool require_initialized = false;
    event_manager_t::get(require_initialized).shutdown();
}

void gaia::rules::subscribe_rule(
    gaia_type_t gaia_type,
    event_type_t event_type,
    const field_position_list_t& fields,
    const rule_binding_t& rule_binding)
{
    event_manager_t::get().subscribe_rule(gaia_type, event_type, fields, rule_binding);
}

bool gaia::rules::unsubscribe_rule(
    gaia_type_t gaia_type,
    event_type_t event_type,
    const field_position_list_t& fields,
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
    const field_position_t* field,
    subscription_list_t& subscriptions)
{
    event_manager_t::get().list_subscribed_rules(ruleset_name, gaia_type, event_type, field, subscriptions);
}

// Note: this function is not used anywhere outside this module hence the linker will not export
// it defined into it's own file. The linker does not consider single functions but entire object
// files for export (unless specified otherwise). Since this file is used outside of this module,
// putting this function here will make the linker export it.
last_operation_t gaia::rules::rule_context_t::last_operation(gaia_type_t other_gaia_type) const
{
    last_operation_t operation = last_operation_t::none;

    if (other_gaia_type != gaia_type)
    {
        return operation;
    }

    switch (event_type)
    {
    //TODO[GAIAPLAT-445] We don't expose deleted row events
    //case db::triggers::event_type_t::row_delete:
    //    operation = last_operation_t::row_delete;
    //    break;
    case db::triggers::event_type_t::row_insert:
        operation = last_operation_t::row_insert;
        break;
    case db::triggers::event_type_t::row_update:
        operation = last_operation_t::row_update;
        break;
    default:
        // Ignore other event types.
        break;
    }

    return operation;
}
