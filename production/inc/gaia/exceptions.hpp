/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

// Export all symbols declared in this file.
#pragma GCC visibility push(default)

#include <sstream>

#include "gaia/exception.hpp"

namespace gaia
{
/**
 * \addtogroup Gaia
 * @{
 */

namespace common
{
/**
 * \addtogroup common
 * @{
 */

/**
 * An exception class used for unexpected library API failures that return error codes.
 */
class api_error : public gaia_exception
{
public:
    api_error(const char* api_name, int error_code);
};

/**
 * An exception class used for to indicate invalid Gaia configuration settings.
 */
class configuration_error : public gaia_exception
{
public:
    explicit configuration_error(const char* key, int value);
    explicit configuration_error(const char* filename);
};

/*@}*/
} // namespace common

namespace db
{
/**
 * \addtogroup db
 * @{
 */

namespace index
{
/**
 * \addtogroup index
 * @{
 */

/**
 * An exception class used for to indicate the violation of a UNIQUE constraint.
 */
class unique_constraint_violation : public common::gaia_exception
{
public:
    unique_constraint_violation()
    {
        m_message = "UNIQUE constraint violation.";
    }

    explicit unique_constraint_violation(const char* index_name, const char* table_name)
    {
        std::stringstream message;
        message
            << "UNIQUE constraint violation for index: '" << index_name
            << "' of table '" << table_name << "'.";
        m_message = message.str();
    }
};

/*@}*/
} // namespace index

/*@}*/
} // namespace db

/*@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
