/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/common/retail_assert.hpp"

#include <sstream>

#include <gaia_spdlog/fmt/fmt.h>

namespace gaia
{
namespace common
{

retail_assertion_failure::retail_assertion_failure(const std::string& message)
    : gaia_exception(message)
{
}

void throw_retail_assertion_failure(const char* message, const char* file, size_t line, const char* function)
{
    throw retail_assertion_failure(
        gaia_fmt::format("Assertion failed in {}::{}(): line {}: {}", file, function, line, message));
}

} // namespace common
} // namespace gaia
