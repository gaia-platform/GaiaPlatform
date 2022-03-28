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

// The ASSERT macros defined in this file are used for internal validation checks
// that are meant to be performed in both debug and retail builds.
//
// These ASSERTs provide a mechanism for failing execution as soon as an issue is detected,
// which should surface errors early on and should prevent more expensive investigations.
//
// A way to disable these ASSERTs (via corresponding defines) is provided mainly as a mechanism
// for determining if they contribute any negative execution impact.
//
// The ASSERT message is wrapped within an if that is evaluated only if the condition is false.
// For this reason is optimal to put string concatenation within the ASSERT, eg.
// ASSERT_PRECONDITION(gaia_fmt::format("Message {}", 123).c_str());

#define DISABLE_ASSERTS

#ifdef DISABLE_ASSERTS
#define DISABLE_ASSERT_PRECONDITION
#define DISABLE_ASSERT_INVARIANT
#define DISABLE_ASSERT_POSTCONDITION
#endif

// ASSERT_PRECONDITION is meant for validating conditions that should hold when a function is called.
//
// This should be used to validate that internal functions are called with the correct parameters
// and in the correct context.
//
// NOTE: This assert should not be used within public functions. Public functions should expect to
// be called incorrectly and should handle such incorrect calls with regular errors or exceptions.
#ifdef DISABLE_ASSERT_PRECONDITION
#define ASSERT_PRECONDITION(c, m)
#else
#define ASSERT_PRECONDITION(c, m)                                                      \
    if (__builtin_expect(!static_cast<bool>(c), 0))                                    \
    {                                                                                  \
        gaia::common::throw_retail_assertion_failure(m, __FILE__, __LINE__, __func__); \
    }
#endif

// ASSERT_INVARIANT is meant for validating conditions that should hold internally,
// during the execution of a function.
//
// This is arguably the most important ASSERT to use, because it can surface
// algorithmic errors which would otherwise be very difficult to detect.
#ifdef DISABLE_ASSERT_INVARIANT
#define ASSERT_INVARIANT(c, m)
#else
#define ASSERT_INVARIANT(c, m)                                                         \
    if (__builtin_expect(!static_cast<bool>(c), 0))                                    \
    {                                                                                  \
        gaia::common::throw_retail_assertion_failure(m, __FILE__, __LINE__, __func__); \
    }
#endif

// ASSERT_POSTCONDITION is meant for validating conditions that should hold after a function
// has completed execution.
//
// This should be used to validate that functions produce the expected outcome.
#ifdef DISABLE_ASSERT_POSTCONDITION
#define ASSERT_POSTCONDITION(c, m)
#else
#define ASSERT_POSTCONDITION(c, m)                                                     \
    if (__builtin_expect(!static_cast<bool>(c), 0))                                    \
    {                                                                                  \
        gaia::common::throw_retail_assertion_failure(m, __FILE__, __LINE__, __func__); \
    }
#endif

// ASSERT_UNREACHABLE is meant for validating that a section of code can never be reached.
//
// Because ASSERT_UNREACHABLE results in an unconditional failure,
// there should never be a need to disable such an assert - even for debugging purposes.
#define ASSERT_UNREACHABLE(m) \
    gaia::common::throw_retail_assertion_failure(m, __FILE__, __LINE__, __func__)

/**
 * Thrown when a retail assert check has failed.
 */
class retail_assertion_failure : public gaia_exception
{
public:
    explicit retail_assertion_failure(const std::string& message)
        : gaia_exception(message)
    {
    }
};

/**
 * Retail asserts are meant for important conditions that indicate internal errors.
 * They help us surface issues early, at the source.
 *
 * This function should only be called through the various assert macros,
 * so that it gets passed the correct information about the point of call.
 *
 * The message parameter is typed const char * to encourage passing string
 * literals rather than dynamically allocated strings.
 */
__attribute__((noreturn)) inline void throw_retail_assertion_failure(
    const char* message, const char* file, size_t line, const char* function)
{
    std::stringstream message_stream;
    message_stream << "Assertion failed in " << file << "::" << function << "(): line " << line << ": " << message;
    throw retail_assertion_failure(message_stream.str());
}

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
