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
namespace events 
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
     * Do not allow assignment or copying; this class is a singleton.
     */
    event_manager_t(event_manager_t&) = delete;
    void operator=(event_manager_t const&) = delete;

    /**
     * Return the singleton static instance.
     */
    static event_manager_t& get();
    
    /**
     * Event APIs
     */
    gaia::common::error_code_t log_event(
      gaia::common::gaia_base * row, 
      gaia::common::gaia_type_t gaia_type,
      event_type type, 
      event_mode mode);

    gaia::common::error_code_t log_event(event_type type, event_mode mode);
    
    /**
     * Rule APIs
     */ 
    gaia::common::error_code_t subscribe_rule(
      gaia::common::gaia_type_t gaia_type, 
      event_type type, 
      const gaia::rules::rule_binding_t& rule_binding);

    gaia::common::error_code_t subscribe_rule(
      event_type type, 
      const gaia::rules::rule_binding_t& rule_binding);

    gaia::common::error_code_t unsubscribe_rule(
      gaia::common::gaia_type_t gaia_type, 
      event_type type, 
      const gaia::rules::rule_binding_t& rule_binding);

    gaia::common::error_code_t unsubscribe_rule(
      event_type type, 
      const gaia::rules::rule_binding_t& rule_binding);

    void list_subscribed_rules(
      const char* ruleset_name, 
      const gaia::common::gaia_type_t* gaia_type, 
      const event_type* type,
      std::vector<const char *>& rule_names);

private:
    // internal rule binding
    struct _rule_binding_t
    {
      _rule_binding_t(const rules::rule_binding_t& binding);

      std::string ruleset_name;
      std::string rule_name;
      rules::gaia_rule_fn rule;
    };


    // only internal static creation is allowed
    event_manager_t();

    //
    // UNDONE: remove the rule_name from the rule_binding
    // make an internal rule_binding class that is copy constructible
    // can get rid of m_rule_names (just hold in internal rule binding class)
    // unsubscribe has to go by function pointer address ...
    // will still generate an internal rule name; this all gets easier
    // when we generate the function dynamically
    // NEW RULES:  caller must supply name rule name and rule fn pointer together
    // there is a 1:1 mapping between ruleset_name::rule_name and the function pointer
    // we have a global map and then the event lists point into this global registry.
    // it's possible to have many references to the same rule (i.e., same rule bound to multiple events)
    //  

    // Hash table of all rules registered with the system
    // the key is the rulset_name::rule_name.
    std::unordered_map<std::string, std::unique_ptr<_rule_binding_t>> m_rules;

    // List of rules associated with an event.  An event
    // may have multiple rules bound to it.
    typedef std::list<const _rule_binding_t *> rule_list_t;

    // Associates a particular event type to its list of rules.
    typedef std::unordered_map<events::event_type, rule_list_t> events_map_t;


    // Associates events with a specifc type.
    std::unordered_map<common::gaia_type_t, events_map_t> m_table_subscriptions;

    // Transaction events are not bound to any specific type.
    events_map_t m_transaction_subscriptions;

    //std::unordered_set<const char *> m_rule_names;

    // first element of pair is whether the element was found or not
    // second elemment of pair, if first element is true, is whether
    // a different rule is bound to the same key.
    const _rule_binding_t* find_rule(const rules::rule_binding_t& binding);
    gaia::common::error_code_t add_rule(rule_list_t& rules, const rules::rule_binding_t& binding);
    gaia::common::error_code_t remove_rule(rule_list_t& rules, const rules::rule_binding_t& binding);
    
    static bool is_valid_table_event(event_type type);
    static bool is_valid_transaction_event(event_type type);
    static bool is_valid_rule_binding(const rules::rule_binding_t& binding);
    static std::string make_rule_key(const rules::rule_binding_t& binding);
    static events_map_t create_table_event_map();
    static void insert_transaction_events(events_map_t& transaction_map);
};

}
}
