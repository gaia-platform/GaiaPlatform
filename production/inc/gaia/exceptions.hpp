/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstring>

#include <sstream>

#include "gaia/exception.hpp"

// Export all symbols declared in this file.
#pragma GCC visibility push(default)

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
 * An exception class used to indicate invalid Gaia configuration settings.
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

/**
 * An exception class used to indicate that the Gaia server session limit has been exceeded.
 */
class session_limit_exceeded : public common::gaia_exception
{
public:
    session_limit_exceeded()
    {
        m_message = "Server session limit exceeded.";
    }
};

/**
 * A base exception class for any fatal pre-commit validation failure.
 *
 * Any expected pre-commit validation failure exception is expected to extend this class.
 */
class pre_commit_validation_failure : public common::gaia_exception
{
};

namespace index
{
/**
 * \addtogroup index
 * @{
 */

/**
 * An exception class used to indicate the violation of a UNIQUE constraint.
 *
 * Extends pre_commit_validation_failure to indicate that this exception
 * is expected to occur during the pre-commit processing of a transaction.
 */
class unique_constraint_violation : public pre_commit_validation_failure
{
public:
    static constexpr char c_error_description[] = "UNIQUE constraint violation!";

public:
    // This constructor should only be used on client side,
    // to re-throw the exception indicated by the server.
    explicit unique_constraint_violation(const char* error_message)
    {
        m_message = error_message;
    }

    // A violation could be triggered by conflict with a non-committed transaction.
    // If that transaction fails to commit, its record will not exist.
    // This is why no record id is being provided in the message: because it may
    // not correspond to any valid record at the time that the error is investigated.
    unique_constraint_violation(const char* error_table_name, const char* error_index_name)
    {
        std::stringstream message;
        message
            << c_error_description
            << " Cannot insert a duplicate key in table: '" << error_table_name << "', because of the unique constraint of "
            << " index: '" << error_index_name << "'.";
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
