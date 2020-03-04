/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <vector>

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
    bool log_event(
      gaia::common::gaia_base * row, 
      event_type type, 
      event_mode mode);

    bool log_event(event_type type, event_mode mode);
    
    /**
     * Rule APIs
     */ 
    bool subscribe_rule(
      gaia::common::gaia_type_t gaia_type, 
      event_type type, 
      gaia::rules::rule_binding_t& rule_binding);

    bool subscribe_rule(
      event_type type, 
      gaia::rules::rule_binding_t& rule_binding);

    bool unsubscribe_rule(
      gaia::common::gaia_type_t gaia_type, 
      event_type type, 
      const gaia::rules::rule_binding_t& rule_binding);

    bool unsubscribe_rule(
      event_type type, 
      const gaia::rules::rule_binding_t& rule_binding);

    void list_subscribed_rules(
      const char* ruleset_name, 
      const gaia::common::gaia_type_t* gaia_type, 
      const event_type* type,
      std::vector<const char *>& rule_names);

private:
   event_manager_t();
};

}
}
