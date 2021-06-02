// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#ifdef _WIN32
#include <gaia_spdlog/sinks/wincolor_sink.h>
#else
#include <gaia_spdlog/sinks/ansicolor_sink.h>
#endif

#include <gaia_spdlog/details/synchronous_factory.h>

namespace gaia_spdlog {
namespace sinks {
#ifdef _WIN32
using stdout_color_sink_mt = wincolor_stdout_sink_mt;
using stdout_color_sink_st = wincolor_stdout_sink_st;
using stderr_color_sink_mt = wincolor_stderr_sink_mt;
using stderr_color_sink_st = wincolor_stderr_sink_st;
#else
using stdout_color_sink_mt = ansicolor_stdout_sink_mt;
using stdout_color_sink_st = ansicolor_stdout_sink_st;
using stderr_color_sink_mt = ansicolor_stderr_sink_mt;
using stderr_color_sink_st = ansicolor_stderr_sink_st;
#endif
} // namespace sinks

template<typename Factory = gaia_spdlog::synchronous_factory>
std::shared_ptr<logger> stdout_color_mt(const std::string &logger_name, color_mode mode = color_mode::automatic);

template<typename Factory = gaia_spdlog::synchronous_factory>
std::shared_ptr<logger> stdout_color_st(const std::string &logger_name, color_mode mode = color_mode::automatic);

template<typename Factory = gaia_spdlog::synchronous_factory>
std::shared_ptr<logger> stderr_color_mt(const std::string &logger_name, color_mode mode = color_mode::automatic);

template<typename Factory = gaia_spdlog::synchronous_factory>
std::shared_ptr<logger> stderr_color_st(const std::string &logger_name, color_mode mode = color_mode::automatic);

} // namespace gaia_spdlog

#ifdef GAIA_SPDLOG_HEADER_ONLY
#include "stdout_color_sinks-inl.h"
#endif
