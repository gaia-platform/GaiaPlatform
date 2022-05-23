////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <sstream>
#include <string>

#include "assert.hpp"

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

// The DEBUG_ASSERT macros defined in this file are used for internal validation checks
// that are meant to be performed in debug builds only.
//
// This is typically the case when the checks are too expensive to perform in release builds.

#ifdef DEBUG
#define DEBUG_ASSERT_PRECONDITION(c, m)                                               \
    if (__builtin_expect(!static_cast<bool>(c), 0))                                   \
    {                                                                                 \
        gaia::common::throw_debug_assertion_failure(m, __FILE__, __LINE__, __func__); \
    }
#else
#define DEBUG_ASSERT_PRECONDITION(c, m)
#endif

#ifdef DEBUG
#define DEBUG_ASSERT_INVARIANT(c, m)                                                  \
    if (__builtin_expect(!static_cast<bool>(c), 0))                                   \
    {                                                                                 \
        gaia::common::throw_debug_assertion_failure(m, __FILE__, __LINE__, __func__); \
    }
#else
#define DEBUG_ASSERT_INVARIANT(c, m)
#endif

#ifdef DEBUG
#define DEBUG_ASSERT_POSTCONDITION(c, m)                                              \
    if (__builtin_expect(!static_cast<bool>(c), 0))                                   \
    {                                                                                 \
        gaia::common::throw_debug_assertion_failure(m, __FILE__, __LINE__, __func__); \
    }
#else
#define DEBUG_ASSERT_POSTCONDITION(c, m)
#endif

/**
 * Thrown when a debug assert check has failed.
 */
class debug_assertion_failure : public assertion_failure
{
public:
    explicit debug_assertion_failure(const std::string& message)
        : assertion_failure(message)
    {
    }
};

/**
 * This function should only be called through the various assert macros,
 * so that it gets passed the correct information about the point of call.
 *
 * The message parameter is typed const char * to encourage passing string
 * literals rather than dynamically allocated strings.
 */
__attribute__((noreturn)) inline void throw_debug_assertion_failure(
    const char* message, const char* file, size_t line, const char* function)
{
    std::stringstream message_stream;
    message_stream << "Debug assertion failed in " << file << "::" << function << "(): line " << line << ": " << message;
    throw debug_assertion_failure(message_stream.str());
}

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
