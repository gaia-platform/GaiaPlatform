/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <unordered_map>
#include "rules.hpp"
#include "event_guard.hpp"
#include "event_log_gaia_generated.h"
#include "rule_thread_pool.hpp"

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
    
    /**
     * Rule APIs
     */ 
    void init();

    void subscribe_rule(
        gaia::common::gaia_type_t gaia_type,
        event_type_t event_type,
        const field_list_t& fields,
        const rule_binding_t& rule_binding);

    bool unsubscribe_rule(
      gaia::common::gaia_type_t gaia_type, 
      event_type_t event_type,
      const field_list_t& fields,
      const rule_binding_t& rule_binding);

    void unsubscribe_rules();

    void list_subscribed_rules(
      const char* ruleset_name, 
      const gaia::common::gaia_type_t* gaia_type, 
      const event_type_t* event_type,
      const field_list_t& fields,
      subscription_list_t& subscriptions);

    struct gaia_event_t {
        event_type_t event_type; // insert, update, delete, begin, commit, rollback
        gaia_type_t gaia_type; // gaia table type, maybe 0 if event has no associated table
        gaia_id_t  row_id; //row id, may be 0 if if there is no assocated row id
        const uint16_t* column_ids; // list of affected columns, may be null
        uint16_t num_column_ids; // count of affected columsn, may be zero
    };



    /**
    * Internal trigger function that is called by the high level storage engine when
    * a commit occurs.
    * Needs to be thread safe and re-entrant
    * 
    * @param tx_id Transaction id that has just been committed.
    * @param events All the events that were part of this transaction.  May be null if this commit had no events.
    * @param num_events May be 0 if this commit had no events.
    */ 
    void commit_trigger(uint32_t tx_id, gaia_event_t* events, uint16_t num_events);
    void enqueue_invocation(const gaia_event_t* event, const _rule_binding_t* rule_binding);

private:
    // only internal static creation is allowed
    event_manager_t();

    // Internal rule binding to copy the callers
    // rule data and hold on to it.  This will be
    // required for deferred rules later.
    struct _rule_binding_t
    {
        _rule_binding_t() = delete;
        _rule_binding_t(const rules::rule_binding_t& binding);

        std::string ruleset_name;
        std::string rule_name;
        rules::gaia_rule_fn rule;
    };

    // The rules engine must be initialized through an explicit call
    // to gaia::rules::initialize_rules_engine(). If this method
    // is not called then all APIs will fail with a gaia::exception.
    bool m_is_initialized = false;

    // Hash table of all rules registered with the system.
    // The key is the rulset_name::rule_name.
    std::unordered_map<std::string, std::unique_ptr<_rule_binding_t>> m_rules;

    // Track the state of all currently executing log event calls.  
    // In Q1 we don't allow reentrancy.  This can happen if a rule 
    // calls the same log_event while handling the log_event that 
    // invoked the rule.
    database_event_guard_t m_database_event_guard;
    field_event_guard_t m_field_event_guard;

    // List of rules that are invoked when an event is logged.
    typedef std::list<const _rule_binding_t*> rule_list_t;


    // Map from a referenced column id to a rule_list.
    typedef std::unordered_map<uint16_t, rule_list_t> fields_map_t;

    // An event can be bound because it changed a field that is referenced
    // or because the LastOperation system field was referenced.
    struct event_binding_t {
        rule_list_t last_operation_rules; // rules bound to this operation
        fields_map_t fields_map; // field to bound to specific fields for this event
    };

    // Map the event type to the event binding
    typedef std::unordered_map<event_type_t, event_binding_t> events_map_t;


    // List of all rule subscriptions by gaia type, event type,
    // and column if appropriate
    std::unordered_map<common::gaia_type_t, events_map_t> m_subscriptions;

    // Thread pool to handle invocation of rules on
    // N threads.
    rule_thread_pool_t m_invocations;

    const _rule_binding_t* find_rule(const rules::rule_binding_t& binding); 
    void add_rule(rule_list_t& rules, const rules::rule_binding_t& binding);
    bool remove_rule(rule_list_t& rules, const rules::rule_binding_t& binding);

    static inline void check_mode(event_mode_t mode)
    {
        if (event_mode_t::deferred == mode) 
        {
            throw mode_not_supported(mode);
        }
    }

    static inline void check_database_event(event_type_t type)
    {
        if (type >= event_type_t::first_field_event) 
        {
            throw invalid_event_type(type);
        }
    }

    static inline void check_database_event(event_type_t type, const gaia_base_t* row)
    {
        check_database_event(type);

        // Transaction events should have no context supplied.
        if ((type < event_type_t::first_row_event) && row)
        {
            throw invalid_context();
        }

        // Row events must have a row context.
        if ((type >= event_type_t::first_row_event) && !row)
        {
            throw invalid_context(row);
        }
    }

    static inline void check_field_event(event_type_t type)
    {
        if (type < event_type_t::first_field_event)
        {
            throw invalid_event_type(type);
        }
    }
    
    static inline void check_field_event(event_type_t type, const gaia_base_t* row, const char* field)
    {
        check_field_event(type);

        // Field events must have both a row context and field.
        if (!row || !field)
        {
            throw invalid_context(row, field);
        }
    }

    static inline void check_rule_binding(const rule_binding_t& binding)
    {
        if (nullptr == binding.rule 
            || nullptr == binding.rule_name
            || nullptr == binding.ruleset_name)
        {
            throw invalid_rule_binding();
        }
    }

    static inline rule_context_t create_rule_context(const _rule_binding_t* binding, 
        gaia_type_t gaia_type, 
        event_type_t event_type,
        gaia_id_t record)
    {
        return { 
            {binding->ruleset_name.c_str(), binding->rule_name.c_str(), binding->rule},
            gaia_type,
            event_type,
            event_context,
            field
        };
    }

    static bool is_valid_rule_binding(const rules::rule_binding_t& binding);
    static std::string make_rule_key(const rules::rule_binding_t& binding);
    static events_map_t create_database_event_map();
    static events_map_t create_field_event_map();
    static void add_subscriptions(rules::subscription_list_t& subscriptions, 
        const events_map_t& events, 
        gaia::common::gaia_type_t gaia_type,
        const char * field_name,
        const char* ruleset_filter, 
        const event_type_t* event_filter);

    // Overload to log database events to the database.
    static inline void log_to_db(gaia_type_t gaia_type, 
        event_type_t event_type, 
        event_mode_t event_mode,
        gaia_base_t* context,
        bool rules_fired)
    {
        log_to_db(gaia_type, event_type, event_mode, context, nullptr, rules_fired);
    }

    // Overload to log fields events to the database which have 
    // an extra field name argument.
    static void log_to_db(gaia_type_t gaia_type, 
        event_type_t event_type, 
        event_mode_t event_mode,
        gaia_base_t* context,
        const char* field,
        bool rules_fired);
};

} // namespace rules
} // namespace gaia
