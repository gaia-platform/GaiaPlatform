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
 * An exception class for database exceptions that should be communicated to the client.
 */
class database_exception : public common::gaia_exception
{
public:
    explicit database_exception(
        const char* error_message,
        const char* error_table_name,
        const char* error_index_name)
    {
        m_error_message = error_message;
        m_table_name = error_table_name;
        m_index_name = error_index_name;

        std::stringstream message;
        message
            << error_message
            << "\nError context information:";

        if (error_table_name && strlen(error_table_name) > 0)
        {
            message << "\n\tTable: '" << error_table_name << "'";
        }

        if (error_index_name && strlen(error_index_name) > 0)
        {
            message << "\n\tIndex: '" << error_index_name << "'";
        }

        message << "\n";

        m_message = message.str();
    }

    const char* get_message() const
    {
        return m_error_message;
    }

    const char* get_table_name() const
    {
        return m_table_name;
    }

    const char* get_index_name() const
    {
        return m_index_name;
    }

private:
    const char* m_error_message;
    const char* m_table_name;
    const char* m_index_name;
};

/*@}*/
} // namespace db

/*@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
