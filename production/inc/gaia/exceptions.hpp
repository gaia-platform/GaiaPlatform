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

namespace index
{
/**
 * \addtogroup index
 * @{
 */

/**
 * An exception class used to indicate the violation of a UNIQUE constraint.
 */
class unique_constraint_exception : public common::gaia_exception
{
public:
    static constexpr char c_error_message[] = "UNIQUE constraint exception!";

public:
    explicit unique_constraint_exception(const char* error_message)
    {
        m_message = error_message;
    }

    unique_constraint_exception(const char* error_table_name, const char* error_index_name)
    {
        std::stringstream message;
        message
            << c_error_message
            << " The database has detected an attempt to insert a duplicate key in table: '"
            << error_table_name << "', "
            << " index: '" << error_index_name << "'.";
        m_message = message.str();
    }

    static bool has_issued_message(const char* error_message)
    {
        return strlen(error_message) > strlen(c_error_message)
            && strncmp(error_message, c_error_message, strlen(c_error_message)) == 0;
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
