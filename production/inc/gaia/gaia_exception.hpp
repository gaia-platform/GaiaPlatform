/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <exception>
#include <string>

namespace gaia
{
/**
 * \addtogroup Gaia
 * @{
 */
namespace common
{
/**
 * \addtogroup Common
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
