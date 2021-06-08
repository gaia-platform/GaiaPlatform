// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once
#include <gaia_spdlog/cfg/helpers.h>
#include <gaia_spdlog/details/registry.h>

//
// Init log levels using each argv entry that starts with "GAIA_SPDLOG_LEVEL="
//
// set all loggers to debug level:
// example.exe "GAIA_SPDLOG_LEVEL=debug"

// set logger1 to trace level
// example.exe "GAIA_SPDLOG_LEVEL=logger1=trace"

// turn off all logging except for logger1 and logger2:
// example.exe "GAIA_SPDLOG_LEVEL=off,logger1=debug,logger2=info"

namespace gaia_spdlog {
namespace cfg {

// search for GAIA_SPDLOG_LEVEL= in the args and use it to init the levels
inline void load_argv_levels(int argc, const char **argv)
{
    const std::string spdlog_level_prefix = "GAIA_SPDLOG_LEVEL=";
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg.find(spdlog_level_prefix) == 0)
        {
            auto levels_string = arg.substr(spdlog_level_prefix.size());
            helpers::load_levels(levels_string);
        }
    }
}

inline void load_argv_levels(int argc, char **argv)
{
    load_argv_levels(argc, const_cast<const char **>(argv));
}

} // namespace cfg
} // namespace gaia_spdlog
