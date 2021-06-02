// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once
#include <gaia_spdlog/cfg/helpers.h>
#include <gaia_spdlog/details/registry.h>
#include <gaia_spdlog/details/os.h>

//
// Init levels and patterns from env variables GAIA_SPDLOG_LEVEL
// Inspired from Rust's "env_logger" crate (https://crates.io/crates/env_logger).
// Note - fallback to "info" level on unrecognized levels
//
// Examples:
//
// set global level to debug:
// export GAIA_SPDLOG_LEVEL=debug
//
// turn off all logging except for logger1:
// export GAIA_SPDLOG_LEVEL="*=off,logger1=debug"
//

// turn off all logging except for logger1 and logger2:
// export GAIA_SPDLOG_LEVEL="off,logger1=debug,logger2=info"

namespace gaia_spdlog {
namespace cfg {
inline void load_env_levels()
{
    auto env_val = details::os::getenv("GAIA_SPDLOG_LEVEL");
    if (!env_val.empty())
    {
        helpers::load_levels(env_val);
    }
}

} // namespace cfg
} // namespace gaia_spdlog
