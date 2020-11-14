/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <gaia_exception.hpp>

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
class configuration_error : public gaia::common::gaia_exception
{
public:
    configuration_error(const char* key, int value);
};

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
