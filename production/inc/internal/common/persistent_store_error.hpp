/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string.h>

#include <sstream>
#include <string>

#include "gaia_exception.hpp"

namespace gaia
{
namespace db
{

/**
 * Thrown when any API calls to the persistent store return an error.
 */
class persistent_store_error : public gaia_exception
{
public:
    persistent_store_error(const string& message, int code = 0)
        : gaia_exception(message)
    {
        m_code = code;
    }
    int get_code()
    {
        return m_code;
    }

private:
    int m_code;
};

} // namespace db
} // namespace gaia
