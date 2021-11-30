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
class invalid_rule_binding : public common::gaia_exception
{
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
class duplicate_rule : public common::gaia_exception
{
};

/**
 * Thrown when the caller either does not initialize the event manager
 * or attempts to initialize an already initialized event manager.
 */
class initialization_error : public common::gaia_exception
{
};

/**
 * Thrown when the caller provides an invalid subscription.  See the
 * constructor methods for the reasons that his could occur.
 */
class invalid_subscription : public common::gaia_exception
{
};

/**
 * ruleset_not_found : public common::gaia_exception
 */
class ruleset_not_found : public common::gaia_exception
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
