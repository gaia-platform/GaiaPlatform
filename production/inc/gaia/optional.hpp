/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <flatbuffers/stl_emulation.h>

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

// Note: if compiling with C++17 or newer,
// flatbuffers::Optional is aliased to std::optional.
template <typename T_value>
using optional_t = flatbuffers::Optional<T_value>;
using nullopt_t = flatbuffers::nullopt_t;
inline constexpr nullopt_t nullopt = flatbuffers::nullopt;

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
