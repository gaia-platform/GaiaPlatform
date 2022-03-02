/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/exceptions.hpp"

// Export all symbols declared in this file.
#pragma GCC visibility push(default)

/**
 * @addtogroup gaia_rules_exceptions
 * @{
 */

namespace gaia
{
namespace rules
{

/**
 * @brief An incomplete rule_binding_t structure was provided.
 *
 * The system needs the function pointer, rule_name, and ruleset_name to
 * be provided by the caller.
 */
class invalid_rule_binding : public common::gaia_exception
{
};

/**
 * @brief A duplicated rule was detected.
 *
 * This can occur in two situations.
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
 * @brief The caller either did not initialize the event manager
 * or tried to initialize an already initialized event manager.
 */
class initialization_error : public common::gaia_exception
{
};

/**
 * @brief The caller provided an invalid subscription.
 *
 * See the constructor methods for the reasons why his could occur.
 */
class invalid_subscription : public common::gaia_exception
{
};

/**
 * @brief The specified ruleset could not be found.
 */
class ruleset_not_found : public common::gaia_exception
{
public:
    explicit ruleset_not_found(const char* ruleset_name);
};

} // namespace rules
} // namespace gaia

/**@}*/ // @addtogroup gaia_rules_exceptions

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
