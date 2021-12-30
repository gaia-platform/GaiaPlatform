/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <exception>
#include <string>

// Export all symbols declared in this file.
#pragma GCC visibility push(default)

namespace gaia
{
/**
 * \addtogroup gaia
 * @{
 */
namespace common
{
/**
 * \addtogroup common
 * @{
 */

/**
 * Root class for Gaia exceptions.
 */
class gaia_exception : public std::exception
{
protected:
    std::string m_message;

public:
    gaia_exception() = default;

    explicit gaia_exception(const std::string& message)
    {
        m_message = message;
    }

    const char* what() const noexcept override
    {
        return m_message.c_str();
    }
};

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
