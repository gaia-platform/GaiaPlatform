/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <vector>

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
 * The application must provide an implementation of initialize_rules().  This
 * function is invoked when the event manager singleton is created.
 */ 
extern "C" void initialize_rules();

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
    const char * ruleset_name;
    const char * rule_name;
    gaia::common::gaia_type_t gaia_type;
    event_type_t type;
};

/**
 * Caller must provide an instance of this type when using the list_subscribed_rules
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
    context_base_t(const rule_binding_t& binding, event_type_t type) 
    : event_type(type), rule_binding(binding) {}
    virtual ~context_base_t() = default;

    context_base_t() = delete;

    const event_type_t event_type;
    const rule_binding_t& rule_binding;
};

/**
 * A transaction context only has the event type for now.  Currently
 * the system does nothing with this event type.
 */
class transaction_context_t : public context_base_t
{
public:
    transaction_context_t(const rule_binding_t& binding, event_type_t type) 
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
        event_type_t a_type,
        gaia::common::gaia_type_t a_gaia_type,
        gaia::common::gaia_base* a_row)
    : context_base_t(binding, a_type)
    , gaia_type(a_gaia_type)
    , row(a_row) {}

    gaia::common::gaia_type_t gaia_type;    
    gaia::common::gaia_base* row;
};

/**
 * Thrown when the caller provides an incomplete rule_binding_t structure.
 * 
 * The system needs the function pointer, rule_name, and ruleset_name to
 * be provided by the caller.
 */ 
class invalid_rule_binding: public gaia::common::gaia_exception
{
public:
    invalid_rule_binding()
    {
        m_message = "Invalid rule binding. "
            "Verify that the ruleset_name, rule_name and rule are provided.";
    }
};

/**
 * Thrown under two circumstances.  
 * 
 * First, the ruleset_name and rule_name must be unique across the system.  
 * If a caller submits a rule_binding that generates the same key but has 
 * a different rule definition then this exception is thrown.  
 * 
 * Second, if a user attempts to subscribe the same rule to the same 
 * gaia type and event  type then this event is thrown.  
 */ 
class duplicate_rule: public gaia::common::gaia_exception
{
public:
    duplicate_rule(const rule_binding_t& binding, bool duplicate_key)
    {
        std::stringstream strs;
        m_message = strs.str();

        if (duplicate_key)
        {
            strs << binding.ruleset_name << "::"
                << binding.rule_name 
                << " already subscribed with the same key "
                "but diffrent rule function.";
        }
        else
        {
            strs << binding.ruleset_name << "::"
                << binding.rule_name 
                << " already subscribed to the same rule list.";
        }

        m_message = strs.str();
        
    }
};


/**
 * Subscribes this rule to the specified table event scoped to the gaia_type. 
 *  
 * Note that it is valid to bind multiple different rules to the same event.  
 * It is also valid to bind the same rule to multiple different events.
 * 
 * @param gaia_type table type to bind the rule to
 * @param event_type the table event type to bind this rule to
 * @param rule_binding caller-supplied rule information; this call will populate rule_name
 * @throw invalid_event_type
 * @throw invalid_rule_binding
 * @throw duplicate_rule
 */
void subscribe_table_rule(
    gaia::common::gaia_type_t gaia_type, 
    event_type_t event_type, 
    const rule_binding_t& rule_binding);

/**
 * Subscribes this rule to the specified transaction event.  
 * 
 * It is legal to bind multiple different rules to the same event.  
 * It is also possible to bind the same rule to multiple different events.
 * 
 * @param type the transaction event type to bind this rule to
 * @param rule_binding caller-supplied rule information; this call will populate rule_name
 * @throw invalid_event_type
 * @throw invalid_rule_binding
 * @throw duplicate_rule
 */
void subscribe_transaction_rule(
    event_type_t type, 
    const rule_binding_t& rule_binding);

/**
 * Unsubscribes this rule from the specified table event scoped by the gaia_type.
 * 
 * @param gaia_type table type to bind the rule to
 * @param type the event type to bind this rule to
 * @param rule_binding caller-supplied rule information
 * @return true if the rule was unsubscribed; false otherwise.
 * @throw invalid_event_type
 * @throw invalid_rule_binding
 */
bool unsubscribe_table_rule(
    gaia::common::gaia_type_t gaia_type, 
    event_type_t type, 
    const rule_binding_t& rule_binding);

/**
 * Unsubscribes this rule from the specified transaction event.  
 * 
 * @param type the transaction event type to bind this rule to
 * @param rule_binding caller-supplied rule information
 * @return true if the rule was unsubscribed; false otherwise.
 * @throw invalid_event_type
 * @throw invalid_rule_binding
 */
bool unsubscribe_transaction_rule(event_type_t type,
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
    const event_type_t* type, 
    list_subscriptions_t& subscriptions);

/*@}*/
}
/*@}*/
}
