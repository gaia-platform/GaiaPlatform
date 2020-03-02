/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "rules.hpp"

#include <vector>

namespace gaia 
{
namespace events 
{

/**
 * Implementation class for event and rule APIs defined
 * in events.hpp and rules.hpp respectively.  See documentation
 * for this API in those headers.  
 * 
 */
class event_manager_t
{
public:
    event_manager_t();
    ~event_manager_t();

    /**
     * Do not allow assingment or copying; this class is a singleton.
     * 
     */
    event_manager_t(event_manager_t&) = delete;
    void operator=(event_manager_t const&) = delete;

    /**
     * return the singleton static instance
     * 
     */
    static event_manager_t& get();
    
    /**
     * event APIs
     * 
     */
    bool log_event(gaia::api::gaia_base * row, event_type type, event_mode mode);
    bool log_event(event_type type, event_mode mode);
    
    /**
     * transaction APIs
     * 
     */ 
    bool subscribe_rule(gaia::api::gaia_type_t gaia_type, event_type type, gaia::rules::rule_binding_t& rule_binding);
    bool subscribe_rule(event_type type, gaia::rules::rule_binding_t& rule_binding);
    bool unsubscribe_rule(gaia::api::gaia_type_t gaia_type, event_type type, const gaia::rules::rule_binding_t& rule_binding);
    bool unsubscribe_rule(event_type type, const gaia::rules::rule_binding_t& rule_binding);
    void list_subscribed_rules(const char * ruleset_name, const gaia::api::gaia_type_t * gaia_type, const event_type * type,
        std::vector<const char *>& rule_names);

private:
};

}
}

