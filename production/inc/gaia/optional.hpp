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

// For simplicity, we use the flatbuffers implementation of Optional.
// flatbuffers::Optional uses std::optional when available or falls back to a
// trivial implementation of optional when std::optional is not available.
// By using flatbuffers::Optional we don't have to convert back and forth between
// flatbuffers::Optional and gaia::common::optional_t.
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
