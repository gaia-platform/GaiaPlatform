////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <flatbuffers/stl_emulation.h>

// Export all symbols declared in this file.
#pragma GCC visibility push(default)

namespace gaia
{
/**
 * @addtogroup gaia
 * @{
 */
namespace common
{
/**
 * @addtogroup common
 * @{
 */

// Note: if compiling with C++17 or newer,
// flatbuffers::Optional is aliased to std::optional.
template <typename T_value>
using optional_t = flatbuffers::Optional<T_value>;
using nullopt_t = flatbuffers::nullopt_t;
inline constexpr nullopt_t nullopt = flatbuffers::nullopt;

/**@}*/
} // namespace common
/**@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
