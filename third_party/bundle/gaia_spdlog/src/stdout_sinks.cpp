// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef GAIA_SPDLOG_COMPILED_LIB
#error Please define GAIA_SPDLOG_COMPILED_LIB to compile this file.
#endif

#include <mutex>

#include <gaia_spdlog/details/null_mutex.h>
#include <gaia_spdlog/async.h>
#include <gaia_spdlog/sinks/stdout_sinks-inl.h>

template class GAIA_SPDLOG_API gaia_spdlog::sinks::stdout_sink_base<gaia_spdlog::details::console_mutex>;
template class GAIA_SPDLOG_API gaia_spdlog::sinks::stdout_sink_base<gaia_spdlog::details::console_nullmutex>;
template class GAIA_SPDLOG_API gaia_spdlog::sinks::stdout_sink<gaia_spdlog::details::console_mutex>;
template class GAIA_SPDLOG_API gaia_spdlog::sinks::stdout_sink<gaia_spdlog::details::console_nullmutex>;
template class GAIA_SPDLOG_API gaia_spdlog::sinks::stderr_sink<gaia_spdlog::details::console_mutex>;
template class GAIA_SPDLOG_API gaia_spdlog::sinks::stderr_sink<gaia_spdlog::details::console_nullmutex>;

template GAIA_SPDLOG_API std::shared_ptr<gaia_spdlog::logger> gaia_spdlog::stdout_logger_mt<gaia_spdlog::synchronous_factory>(const std::string &logger_name);
template GAIA_SPDLOG_API std::shared_ptr<gaia_spdlog::logger> gaia_spdlog::stdout_logger_st<gaia_spdlog::synchronous_factory>(const std::string &logger_name);
template GAIA_SPDLOG_API std::shared_ptr<gaia_spdlog::logger> gaia_spdlog::stderr_logger_mt<gaia_spdlog::synchronous_factory>(const std::string &logger_name);
template GAIA_SPDLOG_API std::shared_ptr<gaia_spdlog::logger> gaia_spdlog::stderr_logger_st<gaia_spdlog::synchronous_factory>(const std::string &logger_name);

template GAIA_SPDLOG_API std::shared_ptr<gaia_spdlog::logger> gaia_spdlog::stdout_logger_mt<gaia_spdlog::async_factory>(const std::string &logger_name);
template GAIA_SPDLOG_API std::shared_ptr<gaia_spdlog::logger> gaia_spdlog::stdout_logger_st<gaia_spdlog::async_factory>(const std::string &logger_name);
template GAIA_SPDLOG_API std::shared_ptr<gaia_spdlog::logger> gaia_spdlog::stderr_logger_mt<gaia_spdlog::async_factory>(const std::string &logger_name);
template GAIA_SPDLOG_API std::shared_ptr<gaia_spdlog::logger> gaia_spdlog::stderr_logger_st<gaia_spdlog::async_factory>(const std::string &logger_name);
