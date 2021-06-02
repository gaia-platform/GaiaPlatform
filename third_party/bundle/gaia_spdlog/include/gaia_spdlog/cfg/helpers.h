// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <gaia_spdlog/common.h>
#include <unordered_map>

namespace gaia_spdlog {
namespace cfg {
namespace helpers {
//
// Init levels from given string
//
// Examples:
//
// set global level to debug: "debug"
// turn off all logging except for logger1: "off,logger1=debug"
// turn off all logging except for logger1 and logger2: "off,logger1=debug,logger2=info"
//
GAIA_SPDLOG_API void load_levels(const std::string &txt);
} // namespace helpers

} // namespace cfg
} // namespace gaia_spdlog

#ifdef GAIA_SPDLOG_HEADER_ONLY
#include "helpers-inl.h"
#endif // GAIA_SPDLOG_HEADER_ONLY
