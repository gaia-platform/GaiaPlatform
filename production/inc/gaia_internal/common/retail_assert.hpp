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

#ifdef DISABLE_ASSERT_PRECONDITION
#define ASSERT_PRECONDITION(c, m)
#else
#define ASSERT_PRECONDITION(c, m) gaia::common::retail_assert_do_not_call_directly(c, m, __FILE__, __LINE__, __func__)
#endif

#ifdef DISABLE_ASSERT_INVARIANT
#define ASSERT_INVARIANT(c, m)
#else
#define ASSERT_INVARIANT(c, m) gaia::common::retail_assert_do_not_call_directly(c, m, __FILE__, __LINE__, __func__)
#endif

#ifdef DISABLE_ASSERT_POSTCONDITION
#define ASSERT_POSTCONDITION(c, m)
#else
#define ASSERT_POSTCONDITION(c, m) gaia::common::retail_assert_do_not_call_directly(c, m, __FILE__, __LINE__, __func__)
#endif

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
 */
inline void retail_assert_do_not_call_directly(
    bool condition, const std::string& message, const char* file, size_t line, const char* function)
{
    if (!condition)
    {
        std::stringstream message_stream;
        message_stream << "Assertion failed in " << file << "::" << function << "(): line " << line << ": " << message;
        throw retail_assertion_failure(message_stream.str());
    }
}

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
