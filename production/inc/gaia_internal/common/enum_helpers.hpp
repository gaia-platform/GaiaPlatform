/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <sstream>
#include <string>

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
 * \addtogroup Common
 * @{
 */

// Converts an `enum class` instance to a value of its underlying type.
template <typename T_enum>
std::underlying_type_t<T_enum> to_integral(T_enum val)
{
    return static_cast<std::underlying_type_t<T_enum>>(val);
}

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
