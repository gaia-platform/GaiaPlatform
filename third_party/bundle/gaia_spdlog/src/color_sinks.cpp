// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef GAIA_SPDLOG_COMPILED_LIB
#error Please define GAIA_SPDLOG_COMPILED_LIB to compile this file.
#endif

#include <mutex>

#include <gaia_spdlog/details/null_mutex.h>
#include <gaia_spdlog/async.h>
//
// color sinks
//
#ifdef _WIN32
#include <gaia_spdlog/sinks/wincolor_sink-inl.h>
template class GAIA_SPDLOG_API gaia_spdlog::sinks::wincolor_sink<gaia_spdlog::details::console_mutex>;
template class GAIA_SPDLOG_API gaia_spdlog::sinks::wincolor_sink<gaia_spdlog::details::console_nullmutex>;
template class GAIA_SPDLOG_API gaia_spdlog::sinks::wincolor_stdout_sink<gaia_spdlog::details::console_mutex>;
template class GAIA_SPDLOG_API gaia_spdlog::sinks::wincolor_stdout_sink<gaia_spdlog::details::console_nullmutex>;
template class GAIA_SPDLOG_API gaia_spdlog::sinks::wincolor_stderr_sink<gaia_spdlog::details::console_mutex>;
template class GAIA_SPDLOG_API gaia_spdlog::sinks::wincolor_stderr_sink<gaia_spdlog::details::console_nullmutex>;
#else
#include "gaia_spdlog/sinks/ansicolor_sink-inl.h"
template class GAIA_SPDLOG_API gaia_spdlog::sinks::ansicolor_sink<gaia_spdlog::details::console_mutex>;
template class GAIA_SPDLOG_API gaia_spdlog::sinks::ansicolor_sink<gaia_spdlog::details::console_nullmutex>;
template class GAIA_SPDLOG_API gaia_spdlog::sinks::ansicolor_stdout_sink<gaia_spdlog::details::console_mutex>;
template class GAIA_SPDLOG_API gaia_spdlog::sinks::ansicolor_stdout_sink<gaia_spdlog::details::console_nullmutex>;
template class GAIA_SPDLOG_API gaia_spdlog::sinks::ansicolor_stderr_sink<gaia_spdlog::details::console_mutex>;
template class GAIA_SPDLOG_API gaia_spdlog::sinks::ansicolor_stderr_sink<gaia_spdlog::details::console_nullmutex>;
#endif

// factory methods for color loggers
#include "gaia_spdlog/sinks/stdout_color_sinks-inl.h"
template GAIA_SPDLOG_API std::shared_ptr<gaia_spdlog::logger> gaia_spdlog::stdout_color_mt<gaia_spdlog::synchronous_factory>(
    const std::string &logger_name, color_mode mode);
template GAIA_SPDLOG_API std::shared_ptr<gaia_spdlog::logger> gaia_spdlog::stdout_color_st<gaia_spdlog::synchronous_factory>(
    const std::string &logger_name, color_mode mode);
template GAIA_SPDLOG_API std::shared_ptr<gaia_spdlog::logger> gaia_spdlog::stderr_color_mt<gaia_spdlog::synchronous_factory>(
    const std::string &logger_name, color_mode mode);
template GAIA_SPDLOG_API std::shared_ptr<gaia_spdlog::logger> gaia_spdlog::stderr_color_st<gaia_spdlog::synchronous_factory>(
    const std::string &logger_name, color_mode mode);

template GAIA_SPDLOG_API std::shared_ptr<gaia_spdlog::logger> gaia_spdlog::stdout_color_mt<gaia_spdlog::async_factory>(
    const std::string &logger_name, color_mode mode);
template GAIA_SPDLOG_API std::shared_ptr<gaia_spdlog::logger> gaia_spdlog::stdout_color_st<gaia_spdlog::async_factory>(
    const std::string &logger_name, color_mode mode);
template GAIA_SPDLOG_API std::shared_ptr<gaia_spdlog::logger> gaia_spdlog::stderr_color_mt<gaia_spdlog::async_factory>(
    const std::string &logger_name, color_mode mode);
template GAIA_SPDLOG_API std::shared_ptr<gaia_spdlog::logger> gaia_spdlog::stderr_color_st<gaia_spdlog::async_factory>(
    const std::string &logger_name, color_mode mode);
