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
    bool log_event(
      gaia::common::gaia_base * row, 
      gaia::common::gaia_type_t gaia_type,
      event_type_t type, 
      event_mode_t mode);

    bool log_event(event_type_t type, event_mode_t mode);
    
    /**
     * Rule APIs
     */ 
    void subscribe_rule(
      gaia::common::gaia_type_t gaia_type, 
      event_type_t event_type, 
      const rule_binding_t& rule_binding);

    void subscribe_rule(
      event_type_t event_type, 
      const rule_binding_t& rule_binding);

    bool unsubscribe_rule(
      gaia::common::gaia_type_t gaia_type, 
      event_type_t event_type, 
      const rule_binding_t& rule_binding);

    bool unsubscribe_rule(
      event_type_t event_type, 
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
    // that an event may fire more than one rule.
    typedef std::unordered_map<event_type_t, rule_list_t> events_map_t;

    // List of all rule subscriptions for tables.  This holds the rules bound
    // to events on each different Gaia type registered in the system.
    std::unordered_map<common::gaia_type_t, events_map_t> m_table_subscriptions;

    // Unlike table events, transaction events are not scoped by any
    // Gaia type.  They are top level so no map from gaia_type_t to 
    // events_map_t is required.
    events_map_t m_transaction_subscriptions;

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

    static inline void check_table_event(event_type_t type)
    {
      if (!(type == event_type_t::col_change 
        || type == event_type_t::row_delete 
        || type == event_type_t::row_insert 
        || type == event_type_t::row_update))
      {
        throw invalid_event_type(type);
      }
    }

    static inline void check_transaction_event(event_type_t type)
    {
      if (!(type == event_type_t::transaction_begin
        || type == event_type_t::transaction_commit
        || type == event_type_t::transaction_rollback))
      {
        throw invalid_event_type(type);
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
