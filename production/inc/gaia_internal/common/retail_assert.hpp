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

#ifdef DISABLE_RETAIL_ASSERTS
#define retail_assert(c, m) no_op()
#else
#define retail_assert(c, m) retail_assert_do_not_call_directly(c, m, __FILE__, __LINE__, __func__)
#endif

/**
 * A no-operation function.
 *
 * When we want to disable retail_assert calls, we'll call this function instead,
 * so that these calls will be optimized out by the compiler.
 *
 * We need a function because retail_assert calls can be prefixed with a namespace
 * qualifier and those qualifiers have to precede a function call.
 */
inline void no_op()
{
}

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
 * This function should only be called through the retail_assert macro,
 * so that it gets passed the correct information about the point of call.
 */
inline void retail_assert_do_not_call_directly(
    bool condition, const std::string& message, const char* file, size_t line, const char* function)
{
    if (!condition)
    {
        std::stringstream message_stream;
        message_stream << message << "\nFile: " << file << ".\nLine: " << line << ".\nFunction: " << function << ".";
        throw retail_assertion_failure(message_stream.str());
    }
}

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
