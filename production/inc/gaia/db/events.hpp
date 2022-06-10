////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <cstdint>

// Export all symbols declared in this file.
#pragma GCC visibility push(default)

/**
 * @defgroup triggers triggers
 * @ingroup db
 * Triggers namespace
 */

namespace gaia
{
/**
 * @addtogroup gaia
 * @{
 */
namespace db
{
/**
 * @addtogroup db
 * @{
 */
namespace triggers
{
/**
 * @addtogroup triggers
 * @{
 */

/**
 * @brief Identifies the type of database event.
 */
enum class event_type_t : uint32_t
{
    not_set = 0,

    // Row events.
    row_update = 1 << 0,
    row_insert = 1 << 1,
};

/**
 * @brief Returns a string representation of the event type.
 */
const char* event_type_name(event_type_t event_type);

/**@}*/
} // namespace triggers
/**@}*/
} // namespace db
/**@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
