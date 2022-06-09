////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <atomic>
#include <list>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

#include "gaia_internal/db/triggers.hpp"
#include "gaia_internal/rules/rules_config.hpp"
#include "gaia_internal/rules/rules_test_helpers.hpp"

#include "rule_checker.hpp"
#include "rule_thread_pool.hpp"
#include "serial_group_manager.hpp"

namespace gaia
{
namespace rules
{

/**
 * Implementation class for event and rule APIs defined
 * in events.hpp and rules.hpp respectively.  See documentation
 * for this API in those headers.
 */
class event_manager_t
{
public:
    /**
     * Event manager scaffolding to ensure we have one global static instance.
     * Do not allow assignment or copying; this class is a singleton.
     */
    event_manager_t(event_manager_t&) = delete;
    void operator=(event_manager_t const&) = delete;
    static event_manager_t& get(bool require_initialized = true);

    /**
     * Rule APIs
     */
    void init();

    void shutdown();

    void subscribe_rule(
        common::gaia_type_t gaia_type,
        db::triggers::event_type_t event_type,
        const common::field_position_list_t& fields,
        const rule_binding_t& rule_binding);

    bool unsubscribe_rule(
        common::gaia_type_t gaia_type,
        db::triggers::event_type_t event_type,
        const common::field_position_list_t& fields,
        const rule_binding_t& rule_binding);

    void unsubscribe_rules();

    void list_subscribed_rules(
        const char* ruleset_name,
        const common::gaia_type_t* gaia_type,
        const db::triggers::event_type_t* event_type,
        const common::field_position_t* field,
        subscription_list_t& subscriptions);

private:
    // Internal rule binding to copy the callers rule data and hold on to it.
    struct _rule_binding_t
    {
        static std::string make_key(const char* ruleset_name, const char* rule_name);
        _rule_binding_t(const rules::rule_binding_t& binding);
        _rule_binding_t(
            const char* ruleset_name,
            const char* rule_name,
            gaia_rule_fn rule,
            uint32_t line_number,
            const char* serial_group_name = nullptr);

        std::string ruleset_name;
        std::string rule_name;
        std::string log_rule_name;
        rules::gaia_rule_fn rule;
        uint32_t line_number;
        std::string serial_group_name;
    };

    // The rules engine must be initialized through an explicit call
    // to gaia::rules::initialize_rules_engine(). If this method
    // is not called then all APIs will fail with a gaia::exception.
    std::atomic_bool m_is_initialized = false;

    // Protect initialization and shutdown from happening concurrently.
    // Note that the public rules engine APIs are not designed to be
    // thread safe.
    std::recursive_mutex m_init_lock;

    // Hash table of all rules registered with the system.
    // The key is the ruleset_name::rule_name.
    std::unordered_map<std::string, std::unique_ptr<_rule_binding_t>> m_rules;

    // List of rules that are invoked when an event is logged.
    typedef std::list<const _rule_binding_t*> rule_list_t;

    // Map from a referenced column id to a rule_list.
    typedef std::unordered_map<common::field_position_t, rule_list_t> fields_map_t;

    // An event can be bound because it changed a field that is referenced
    // or because the LastOperation system field was referenced.
    struct event_binding_t
    {
        rule_list_t last_operation_rules; // rules bound to this operation
        fields_map_t fields_map; // referenced fields of this type
        std::shared_ptr<rule_thread_pool_t::serial_group_t> serial_group; // serial stream associated with this operation
    };

    // Map the event type to the event binding.
    typedef std::unordered_map<db::triggers::event_type_t, event_binding_t> events_map_t;

    // List of all rule subscriptions by gaia type, event type,
    // and column if appropriate
    std::unordered_map<common::gaia_type_t, events_map_t> m_subscriptions;

    // Helper class to verify rule subscriptions against
    // the catalog.
    std::unique_ptr<rule_checker_t> m_rule_checker;

    // Helper class to manager gathering and logging performance statistics
    // for both the rules engine scheduler and individual rules.
    std::unique_ptr<rule_stats_manager_t> m_stats_manager;

    // Thread pool to handle invocation of rules on N threads.
    // This declaration comes last since worker threads can use any of the above
    // members, so the thread pool must be initialized last and destroyed first.
    std::unique_ptr<rule_thread_pool_t> m_invocations;

    // Commit trigger function pointer that the database calls
    // whenever a transaction is committed.
    gaia::db::triggers::commit_trigger_fn m_trigger_fn;

    // Ensures that all rules run in a ruleset marked with the 'serial_group(stream_name)'
    // attribute are run sequentially.
    serial_group_manager_t m_serial_group_manager;

private:
    // Only internal static creation is allowed.
    event_manager_t() = default;

    // Internal initialization function called by the system.
    friend void gaia::rules::initialize_rules_engine(std::shared_ptr<cpptoml::table>& root_config);

    // Test helper methods.  These are just the friend declarations.  These methods are
    // implemented in a separate source file that must be compiled into the test.
    friend void gaia::rules::test::initialize_rules_engine(const event_manager_settings_t& settings);
    friend void gaia::rules::test::commit_trigger(const db::triggers::trigger_event_t*, size_t count_events);
    friend void gaia::rules::test::wait_for_rules_to_complete();

    // Well known trigger function called by the database after commit.
    void commit_trigger(const db::triggers::trigger_event_list_t& event_list);
    void process_last_operation_events(
        event_binding_t& binding,
        const db::triggers::trigger_event_t& event,
        std::chrono::steady_clock::time_point& start_time);
    void process_field_events(
        event_binding_t& binding,
        const db::triggers::trigger_event_t& event,
        std::chrono::steady_clock::time_point& start_time);
    void init(const event_manager_settings_t& settings);
    const _rule_binding_t* find_rule(const rules::rule_binding_t& binding);
    bool add_rule(rule_list_t& rules, const rules::rule_binding_t& binding);
    bool remove_rule(rule_list_t& rules, const rules::rule_binding_t& binding);
    void enqueue_invocation(
        const db::triggers::trigger_event_t& event,
        const _rule_binding_t* rule_binding,
        std::chrono::steady_clock::time_point& start_time,
        std::shared_ptr<rule_thread_pool_t::serial_group_t>& serial_group);
    void check_subscription(db::triggers::event_type_t event_type, const common::field_position_list_t& fields);
    static void check_rule_binding(const rule_binding_t& binding);
    static void add_subscriptions(
        rules::subscription_list_t& subscriptions,
        const rule_list_t& rules,
        common::gaia_type_t gaia_type,
        db::triggers::event_type_t event_type,
        common::field_position_t field,
        const char* ruleset_filter);
    static void initialize_rule_tables();
};

} // namespace rules
} // namespace gaia
