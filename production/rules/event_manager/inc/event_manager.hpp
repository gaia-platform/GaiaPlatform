/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <vector>
#include <list>
#include <unordered_map>
#include <memory>

#include "rules.hpp"

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
    ~event_manager_t();

    /**
     * Event manager scaffolding to ensure we have one global static instance.
     * Do not allow assignment or copying; this class is a singleton.
     */
    event_manager_t(event_manager_t&) = delete;
    void operator=(event_manager_t const&) = delete;
    static event_manager_t& get();
    
    /**
     * Event APIs
     */
    gaia::common::error_code_t log_event(
      gaia::common::gaia_base * row, 
      gaia::common::gaia_type_t gaia_type,
      event_type_t type, 
      event_mode mode);

    gaia::common::error_code_t log_event(event_type_t type, event_mode mode);
    
    /**
     * Rule APIs
     */ 
    gaia::common::error_code_t subscribe_rule(
      gaia::common::gaia_type_t gaia_type, 
      event_type_t type, 
      const rule_binding_t& rule_binding);

    gaia::common::error_code_t subscribe_rule(
      event_type_t type, 
      const rule_binding_t& rule_binding);

    gaia::common::error_code_t unsubscribe_rule(
      gaia::common::gaia_type_t gaia_type, 
      event_type_t type, 
      const rule_binding_t& rule_binding);

    gaia::common::error_code_t unsubscribe_rule(
      event_type_t type, 
      const rule_binding_t& rule_binding);

    void unsubscribe_rules();

    void list_subscribed_rules(
      const char* ruleset_name, 
      const gaia::common::gaia_type_t* gaia_type, 
      const event_type_t* type,
      list_subscriptions_t& subscriptions);

private:
    // Internal rule binding to copy the callers
    // rule data and hold on to it.  This will be
    // required for deferred rules later.
    struct _rule_binding_t
    {
      _rule_binding_t(const rules::rule_binding_t& binding);

      std::string ruleset_name;
      std::string rule_name;
      rules::gaia_rule_fn rule;
    };

    // only internal static creation is allowed
    event_manager_t();

    // Hash table of all rules registered with the system.
    // The key is the rulset_name::rule_name.
    std::unordered_map<std::string, std::unique_ptr<_rule_binding_t>> m_rules;

    // List of rules that are invoked when an event is logged.
    typedef std::list<const _rule_binding_t *> rule_list_t;

    // Associates a particular event type to its list of rules.  This means
    // that an event may fire moree than one rule.
    typedef std::unordered_map<event_type_t, rule_list_t> events_map_t;

    // List of all rule subscriptions for tables.  This holds the rules bound
    // to events on each different Gaia type registered in the system.
    std::unordered_map<common::gaia_type_t, events_map_t> m_table_subscriptions;

    // Unlike table events, transaction events are not scoped by any
    // Gaia type.  They are top level
    events_map_t m_transaction_subscriptions;

    const _rule_binding_t* find_rule(const rules::rule_binding_t& binding);
    gaia::common::error_code_t add_rule(rule_list_t& rules, const rules::rule_binding_t& binding);
    gaia::common::error_code_t remove_rule(rule_list_t& rules, const rules::rule_binding_t& binding);

    static bool is_valid_table_event(event_type_t type);
    static bool is_valid_transaction_event(event_type_t type);
    static bool is_valid_rule_binding(const rules::rule_binding_t& binding);
    static std::string make_rule_key(const rules::rule_binding_t& binding);
    static events_map_t create_table_event_map();
    static void insert_transaction_events(events_map_t& transaction_map);
    static void add_subscriptions(rules::list_subscriptions_t& subscriptions, 
      const events_map_t& events, 
      gaia::common::gaia_type_t gaia_type,
      const char * ruleset_filter, 
      const event_type_t * event_filter);
};

} // namespace events
} // namespace gaia
