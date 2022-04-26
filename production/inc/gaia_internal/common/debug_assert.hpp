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

// The DEBUG_ASSERT macros defined in this file are used for internal validation checks
// that are meant to be performed in debug builds only.
//
// These DEBUG_ASSERTs provide a mechanism for failing execution as soon as an issue is detected,
// which should surface errors early on and should prevent more expensive investigations.
//
// The DEBUG_ASSERT message is wrapped within an if that is evaluated only if the condition is false.
// For this reason, it is optimal to put string concatenation within the ASSERT, eg.
// ASSERT_PRECONDITION(gaia_fmt::format("Message {}", 123).c_str());

// ASSERT_PRECONDITION is meant for validating conditions that should hold when a function is called.
//
// This should be used to validate that internal functions are called with the correct parameters
// and in the correct context.
//
// NOTE: This assert should not be used within public functions. Public functions should expect to
// be called incorrectly and should handle such incorrect calls with regular errors or exceptions.
#ifdef DEBUG
#define DEBUG_ASSERT_PRECONDITION(c, m)                                               \
    if (__builtin_expect(!static_cast<bool>(c), 0))                                   \
    {                                                                                 \
        gaia::common::throw_debug_assertion_failure(m, __FILE__, __LINE__, __func__); \
    }
#else
#define DEBUG_ASSERT_PRECONDITION(c, m)
#endif

// ASSERT_INVARIANT is meant for validating conditions that should hold internally,
// during the execution of a function.
//
// This is arguably the most important ASSERT to use, because it can surface
// algorithmic errors which would otherwise be very difficult to detect.
#ifdef DEBUG
#define DEBUG_ASSERT_INVARIANT(c, m)                                                  \
    if (__builtin_expect(!static_cast<bool>(c), 0))                                   \
    {                                                                                 \
        gaia::common::throw_debug_assertion_failure(m, __FILE__, __LINE__, __func__); \
    }
#else
#define DEBUG_ASSERT_INVARIANT(c, m)
#endif

// ASSERT_POSTCONDITION is meant for validating conditions that should hold after a function
// has completed execution.
//
// This should be used to validate that functions produce the expected outcome.
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
class debug_assertion_failure : public gaia_exception
{
public:
    explicit debug_assertion_failure(const std::string& message)
        : gaia_exception(message)
    {
    }
};

/**
 * Debug asserts are meant for important conditions that indicate internal errors.
 * They help us surface issues early, at the source.
 *
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
