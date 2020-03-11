/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <vector>

#include "error_codes.hpp"
#include "events.hpp"

namespace gaia 
{
/**
 * \addtogroup Gaia
 * @{
 */

namespace rules 
{

/**
 * \addtogroup Rules
 * @{
 * 
 * Provides facilities for subscribing and unsubscribing rules
 * to events.
 */

class context_base_t;

/**
 * All rules must adhere to this prototype.  Since the preamble of the rule
 * is generated by a GaiaProcessor, the context parameter should
 * be cast to the appropriate derived context before use.
 */
typedef void (* gaia_rule_fn)(const context_base_t * context);

/**
 * The caller supplies a rule_binding to subscribe/unsubscribe rules to/from events.
 * The caller must supply the ruleset_name, rule_name, and the function pointer for the rule.
 * The ruleset_name and the rule_name must uniquely identify the rule.
  */
struct rule_binding_t {
    rule_binding_t() 
        : ruleset_name(nullptr)
        , rule_name(nullptr), 
        rule(nullptr) {}
    
    rule_binding_t(
        const char *a_ruleset_name, 
        const char * a_rule_name, 
        gaia_rule_fn a_rule)
        : ruleset_name(a_ruleset_name)
        , rule_name(a_rule_name)
        , rule(a_rule) {}
    
    const char* ruleset_name;
    const char* rule_name;
    gaia_rule_fn rule;
};

/**
 * This type is returned in a caller-supplied vector when
 * the list_rules api is called
 */ 
struct subscription_t {
    subscription_t(const char * a_ruleset_name,
        const char * a_rule_name,
        gaia::common::gaia_type_t a_gaia_type,
        gaia::events::event_type a_type)
    : ruleset_name(a_ruleset_name)
    , rule_name(a_rule_name)
    , gaia_type(a_gaia_type)
    , type(a_type) {}

    const char * ruleset_name;
    const char * rule_name;
    gaia::common::gaia_type_t gaia_type;
    gaia::events::event_type type;
};

/**
 * caller must provide this type when using the list_subscribed_rules
 * method below
 */
typedef std::vector<std::unique_ptr<subscription_t>> list_subscriptions_t;

/**
 * The rule context wraps the event (or data) context (the data upon which 
 * the rule code operates) as well as information about the event and rule 
 * metadata.  In the future the rule context may also maintain the error 
 * state of the rule invocation.  Therefore, the rule contexts map 1:1 to 
 * each rule that is bound to an event.  Data contexts may apply to more
 * than one rule. 
 * 
 * Note:  A single event may be bound to multiple rules or a rule may be bound
 * to multiple events. Rules may also be invoked both synchronously or 
 * asynchronously.  For this reason,events and rules are decoupled in 
 * the system.
 * 
 * The rule binding is included for debugging and may be removed in a later
 * iteration.
 */
class context_base_t 
{
public:            
    context_base_t(const rule_binding_t& binding, events::event_type type) 
    : event_type(type), rule_binding(binding) {}
    virtual ~context_base_t() = default;

    context_base_t() = delete;

    const events::event_type event_type;
    const rule_binding_t& rule_binding;
};

/**
 * A transaction context only has the event type for now.  Currently
 * the system does nothing with this event type.
 */
class transaction_context_t : public context_base_t
{
public:
    transaction_context_t(const rule_binding_t& binding, events::event_type type) 
    : context_base_t(binding, type) {}
};

/**
 * A table context contains the row that the operation affected
 */
class table_context_t : public context_base_t
{
public:
    table_context_t(
        const rule_binding_t& binding, 
        events::event_type a_type,
        gaia::common::gaia_type_t a_gaia_type,
        gaia::common::gaia_base* a_row)
    : context_base_t(binding, a_type)
    , gaia_type(a_gaia_type)
    , row(a_row) {}

    gaia::common::gaia_type_t gaia_type;    
    gaia::common::gaia_base* row;
};


/**
 * Subscribes this rule to the specified table event scoped to the gaia_type. 
 *  
 * Note that it is valid to bind multiple different rules to the same event.  
 * It is also valid to bind the same rule to multiple different events.
 * 
 * @param gaia_type table type to bind the rule to
 * @param type the event type to bind this rule to
 * @param rule_binding caller-supplied rule information; this call will populate rule_name
 * @return error_code_t::success, Or:
 *      error_code_t::invalid_event_type (must be a transaction event)
 *      error_code_t::invalid_rule_binding (no ruleset_name, no rule)
 *      error_code_t::duplicate_rule (if the ruleset_name/rule_name pair already has been subscribed
 *      to the transaction event)
 */
gaia::common::error_code_t subscribe_table_rule(
    gaia::common::gaia_type_t gaia_type, 
    gaia::events::event_type type, 
    const rule_binding_t& rule_binding);


/**
 * Subscribes this rule to the specified transaction event.  
 * 
 * It is legal to bind multiple different rules to the same event.  
 * It is also possible to bind the same rule to multiple different events.
 * 
 * @param type the transaction event type to bind this rule to
 * @param rule_binding caller-supplied rule information; this call will populate rule_name
 * @return error_code_t::success, Or:
 *      error_code_t::invalid_event_type (must be a transaction event)
 *      error_code_t::invalid_rule_binding (no ruleset_name, no rule)
 *      error_code_t::duplicate_rule (if the ruleset_name/rule_name pair already has been subscribed
 *      to the transaction event)
 */
gaia::common::error_code_t subscribe_transaction_rule(
    gaia::events::event_type type, 
    const rule_binding_t& rule_binding);


/**
 * Unsubscribes this rule from the specified table event scoped by the gaia_type.
 * 
 * @param gaia_type table type to bind the rule to
 * @param type the event type to bind this rule to
 * @param rule_binding caller-supplied rule information
 * @return error_code_t::success, or:
 *      error_code_t::invalid_event_type (must be a table event)
 *      error_code_t::invalid_rule_binding (no ruleset_name or no rule_name)
 *      error_code_t::rule_not_found
 */
gaia::common::error_code_t unsubscribe_table_rule(
    gaia::common::gaia_type_t gaia_type, 
    gaia::events::event_type type, 
    const rule_binding_t& rule_binding);

/**
 * Unsubscribes this rule from the specified transaction event.  
 * 
 * @param type the transaction event type to bind this rule to
 * @param rule_binding caller-supplied rule information
 * @return true on success, false on failure. Failure can occure for the following reasons:
 *      invalid event type (must be a transaction event)
 *      invalid rule_binding (no ruleset_name, no rule)
 *      duplicate rule_binding (if the ruleset_name/rule_name pair already has been subscribed
 *      to the transaction event)
 */
gaia::common::error_code_t unsubscribe_transaction_rule(gaia::events::event_type type,
    const rule_binding_t& rule_binding);

/**
 * Unsubscribes all rules that were subscribed from the system.  May be called
 * even if no rules have been subscribed.
 */
void unsubscribe_rules();    

/**
 * List all rules already subscribed to events.  
 * 
 * Enable filtering on ruleset name, gaia_type, and event_type.
 * 
 * @param ruleset_name Scope returned rules to specified rulset if provided.  May be null.
 * @param gaia_type Filter results by the object they refer to.  May be null.
 * @param type Filter by event_type.  May be null. 
 * @param subscriptions Caller provided vector to hold the results.  This method will clear any existing
 *      entries before adding new ones.
 */
void list_subscribed_rules(
    const char* ruleset_name, 
    const gaia::common::gaia_type_t* gaia_type, 
    const gaia::events::event_type* type, 
    list_subscriptions_t& subscriptions);

/*@}*/
}
/*@}*/
}
