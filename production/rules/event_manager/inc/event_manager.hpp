/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <atomic>
#include <list>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

#include "event_manager_test_helpers.hpp"
#include "gaia_event_log.h"
#include "rule_checker.hpp"
#include "rule_thread_pool.hpp"
#include "rules.hpp"
#include "rules_config.hpp"
#include "triggers.hpp"

using namespace gaia::db::triggers;

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
    static event_manager_t& get(bool is_initializing = false);
    static const char* s_gaia_log_event_rule;

    /**
     * Rule APIs
     */
    void init();

    void shutdown();

    void subscribe_rule(
        gaia::common::gaia_type_t gaia_type,
        event_type_t event_type,
        const field_position_list_t& fields,
        const rule_binding_t& rule_binding);

    bool unsubscribe_rule(
        gaia::common::gaia_type_t gaia_type,
        event_type_t event_type,
        const field_position_list_t& fields,
        const rule_binding_t& rule_binding);

    void unsubscribe_rules();

    void list_subscribed_rules(
        const char* ruleset_name,
        const gaia::common::gaia_type_t* gaia_type,
        const event_type_t* event_type, const uint16_t* field,
        subscription_list_t& subscriptions);

private:
    // Internal rule binding to copy the callers
    // rule data and hold on to it.  This will be
    // required for deferred rules later.
    struct _rule_binding_t
    {
        _rule_binding_t(const rules::rule_binding_t& binding);
        _rule_binding_t(const char* a_ruleset_name, const char* a_rule_name, gaia_rule_fn rule);

        std::string ruleset_name;
        std::string rule_name;
        rules::gaia_rule_fn rule;
    };

    // The rules engine must be initialized through an explicit call
    // to gaia::rules::initialize_rules_engine(). If this method
    // is not called then all APIs will fail with a gaia::exception.
    atomic_bool m_is_initialized = false;

    // Protect initialization and shutdown from happening concurrently.
    // Note, that the public rules engine APIs are not designed to be
    // thread safe.
    std::mutex m_init_lock;

    // Hash table of all rules registered with the system.
    // The key is the rulset_name::rule_name.
    std::unordered_map<std::string, std::unique_ptr<_rule_binding_t>> m_rules;

    // List of rules that are invoked when an event is logged.
    typedef std::list<const _rule_binding_t*> rule_list_t;

    // Map from a referenced column id to a rule_list.
    typedef std::unordered_map<uint16_t, rule_list_t> fields_map_t;

    // An event can be bound because it changed a field that is referenced
    // or because the LastOperation system field was referenced.
    struct event_binding_t
    {
        rule_list_t last_operation_rules; // rules bound to this operation
        fields_map_t fields_map; // referenced fields of this type
    };

    // Map the event type to the event binding.
    typedef std::unordered_map<event_type_t, event_binding_t> events_map_t;

    // List of all rule subscriptions by gaia type, event type,
    // and column if appropriate
    std::unordered_map<common::gaia_type_t, events_map_t> m_subscriptions;

    // Thread pool to handle invocation of rules on
    // N threads.
    unique_ptr<rule_thread_pool_t> m_invocations;

    // Helper class to verify rule subscriptions against
    // the catalog.
    unique_ptr<rule_checker_t> m_rule_checker;

    // Helper class to manager gathering and logging performance statistics
    // for both the rules engine scheduler and individual rules.
    unique_ptr<rule_stats_manager_t> m_stats_manager;

private:
    // Only internal static creation is allowed.
    event_manager_t() = default;

    // Internal initialization function called by the system.
    friend void gaia::rules::initialize_rules_engine(shared_ptr<cpptoml::table>& root_config);

    // Test helper methods.  These are just the friend declarations.  These methods are
    // implemented in a separate source file that must be compiled into the test.
    friend void gaia::rules::test::initialize_rules_engine(event_manager_settings_t& settings);
    friend void gaia::rules::test::commit_trigger(gaia_txn_id_t, const trigger_event_t*, size_t count_events);

    // Well known trigger function called by the storage engine after commit.
    void commit_trigger(gaia_txn_id_t txn_id, const trigger_event_list_t& event_list);
    bool process_last_operation_events(
        event_binding_t& binding,
        const trigger_event_t& event,
        std::chrono::steady_clock::time_point& start_time);
    bool process_field_events(
        event_binding_t& binding,
        const trigger_event_t& event,
        std::chrono::steady_clock::time_point& start_time);
    void init(event_manager_settings_t& settings);
    const _rule_binding_t* find_rule(const rules::rule_binding_t& binding);
    void add_rule(rule_list_t& rules, const rules::rule_binding_t& binding);
    bool remove_rule(rule_list_t& rules, const rules::rule_binding_t& binding);
    void enqueue_invocation(const trigger_event_list_t& events, const vector<bool>& rules_invoked_list, std::chrono::steady_clock::time_point& start_time);
    void enqueue_invocation(
        const trigger_event_t& event,
        const _rule_binding_t* rule_binding,
        std::chrono::steady_clock::time_point& start_time);
    void check_subscription(event_type_t event_type, const field_position_list_t& fields);
    static inline void check_rule_binding(const rule_binding_t& binding)
    {
        if (nullptr == binding.rule || nullptr == binding.rule_name || nullptr == binding.ruleset_name)
        {
            throw invalid_rule_binding();
        }
    }
    static bool is_valid_rule_binding(const rules::rule_binding_t& binding);
    static std::string make_rule_key(const rules::rule_binding_t& binding);
    static void add_subscriptions(
        rules::subscription_list_t& subscriptions,
        const rule_list_t& rules,
        gaia::common::gaia_type_t gaia_type,
        event_type_t event_type,
        uint16_t field,
        const char* ruleset_filter);
};

} // namespace rules
} // namespace gaia
