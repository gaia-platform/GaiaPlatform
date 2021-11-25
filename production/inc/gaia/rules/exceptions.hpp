/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/db/events.hpp"
#include "gaia/exceptions.hpp"

// Export all symbols declared in this file.
#pragma GCC visibility push(default)

namespace gaia
{
/**
 * \addtogroup gaia
 * @{
 */

namespace rules
{
/**
 * \addtogroup rules
 * @{
 */

/**
 * Thrown when the caller provides an incomplete rule_binding_t structure.
 *
 * The system needs the function pointer, rule_name, and ruleset_name to
 * be provided by the caller.
 */
class invalid_rule_binding : public gaia::common::gaia_exception
{
public:
    invalid_rule_binding();
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
class duplicate_rule : public gaia::common::gaia_exception
{
public:
    duplicate_rule(const char* ruleset_name, const char* rule_name, bool duplicate_key_found);
};

/**
 * Thrown when the caller either does not initialize the event manager
 * or attempts to initialize an already initialized event manager.
 */
class initialization_error : public gaia::common::gaia_exception
{
public:
    initialization_error();
};

/**
 * Thrown when the caller provides an invalid subscription.  See the
 * constructor methods for the reasons that his could occur.
 */
class invalid_subscription : public gaia::common::gaia_exception
{
public:
    // Table type not found.
    explicit invalid_subscription(common::gaia_type_t gaia_type);
    // Invalid event type specified.
    invalid_subscription(db::triggers::event_type_t event_type, const char* reason);
    // Field not found.
    invalid_subscription(common::gaia_type_t gaia_type, const char* table, uint16_t position);
    // Field not active or has been deprecated
    invalid_subscription(
        common::gaia_type_t gaia_type,
        const char* table,
        uint16_t position,
        const char* field_name,
        bool is_deprecated);
};

/**
 * ruleset_not_found_subscription : public gaia::common::gaia_exception
 */
class ruleset_not_found : public gaia::common::gaia_exception
{
public:
    explicit ruleset_not_found(const char* ruleset_name);
};

/*@}*/
} // namespace rules
/*@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
