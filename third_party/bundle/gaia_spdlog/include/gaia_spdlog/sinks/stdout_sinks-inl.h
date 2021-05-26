// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#ifndef GAIA_SPDLOG_HEADER_ONLY
#include <gaia_spdlog/sinks/stdout_sinks.h>
#endif

#include <gaia_spdlog/details/console_globals.h>
#include <gaia_spdlog/pattern_formatter.h>
#include <memory>

#ifdef _WIN32
// under windows using fwrite to non-binary stream results in \r\r\n (see issue #1675)
// so instead we use ::FileWrite
#include <gaia_spdlog/details/windows_include.h>
#include <fileapi.h> // WriteFile (..)
#include <io.h>      // _get_osfhandle(..)
#include <stdio.h>   // _fileno(..)
#endif               // WIN32

namespace gaia_spdlog {

namespace sinks {

template<typename ConsoleMutex>
GAIA_SPDLOG_INLINE stdout_sink_base<ConsoleMutex>::stdout_sink_base(FILE *file)
    : mutex_(ConsoleMutex::mutex())
    , file_(file)
    , formatter_(details::make_unique<gaia_spdlog::pattern_formatter>())
{
#ifdef _WIN32
    // get windows handle from the FILE* object
    
    handle_ = (HANDLE)::_get_osfhandle(::_fileno(file_));    
        
    // don't throw to support cases where no console is attached,
    // and let the log method to do nothing if (handle_ == INVALID_HANDLE_VALUE).
    // throw only if non stdout/stderr target is requested (probably regular file and not console).
    if (handle_ == INVALID_HANDLE_VALUE && file != stdout && file != stderr)
    {
        throw_spdlog_ex("gaia_spdlog::stdout_sink_base: _get_osfhandle() failed", errno);
    }
#endif // WIN32
}

template<typename ConsoleMutex>
GAIA_SPDLOG_INLINE void stdout_sink_base<ConsoleMutex>::log(const details::log_msg &msg)
{
#ifdef _WIN32
    if (handle_ == INVALID_HANDLE_VALUE)
    {        
        return;
    }
    std::lock_guard<mutex_t> lock(mutex_);
    memory_buf_t formatted;
    formatter_->format(msg, formatted);
    ::fflush(file_); // flush in case there is somthing in this file_ already
    auto size = static_cast<DWORD>(formatted.size());
    DWORD bytes_written = 0;
    bool ok = ::WriteFile(handle_, formatted.data(), size, &bytes_written, nullptr) != 0;
    if (!ok)
    {
        throw_spdlog_ex("stdout_sink_base: WriteFile() failed. GetLastError(): " + std::to_string(::GetLastError()));
    }
#else
    std::lock_guard<mutex_t> lock(mutex_);
    memory_buf_t formatted;
    formatter_->format(msg, formatted);
    ::fwrite(formatted.data(), sizeof(char), formatted.size(), file_);
    ::fflush(file_); // flush every line to terminal
#endif // WIN32    
}

template<typename ConsoleMutex>
GAIA_SPDLOG_INLINE void stdout_sink_base<ConsoleMutex>::flush()
{
    std::lock_guard<mutex_t> lock(mutex_);
    fflush(file_);
}

template<typename ConsoleMutex>
GAIA_SPDLOG_INLINE void stdout_sink_base<ConsoleMutex>::set_pattern(const std::string &pattern)
{
    std::lock_guard<mutex_t> lock(mutex_);
    formatter_ = std::unique_ptr<gaia_spdlog::formatter>(new pattern_formatter(pattern));
}

template<typename ConsoleMutex>
GAIA_SPDLOG_INLINE void stdout_sink_base<ConsoleMutex>::set_formatter(std::unique_ptr<gaia_spdlog::formatter> sink_formatter)
{
    std::lock_guard<mutex_t> lock(mutex_);
    formatter_ = std::move(sink_formatter);
}

// stdout sink
template<typename ConsoleMutex>
GAIA_SPDLOG_INLINE stdout_sink<ConsoleMutex>::stdout_sink()
    : stdout_sink_base<ConsoleMutex>(stdout)
{}

// stderr sink
template<typename ConsoleMutex>
GAIA_SPDLOG_INLINE stderr_sink<ConsoleMutex>::stderr_sink()
    : stdout_sink_base<ConsoleMutex>(stderr)
{}

} // namespace sinks

// factory methods
template<typename Factory>
GAIA_SPDLOG_INLINE std::shared_ptr<logger> stdout_logger_mt(const std::string &logger_name)
{
    return Factory::template create<sinks::stdout_sink_mt>(logger_name);
}

template<typename Factory>
GAIA_SPDLOG_INLINE std::shared_ptr<logger> stdout_logger_st(const std::string &logger_name)
{
    return Factory::template create<sinks::stdout_sink_st>(logger_name);
}

template<typename Factory>
GAIA_SPDLOG_INLINE std::shared_ptr<logger> stderr_logger_mt(const std::string &logger_name)
{
    return Factory::template create<sinks::stderr_sink_mt>(logger_name);
}

template<typename Factory>
GAIA_SPDLOG_INLINE std::shared_ptr<logger> stderr_logger_st(const std::string &logger_name)
{
    return Factory::template create<sinks::stderr_sink_st>(logger_name);
}
} // namespace gaia_spdlog
