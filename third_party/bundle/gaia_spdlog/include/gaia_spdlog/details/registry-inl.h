// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#ifndef GAIA_SPDLOG_HEADER_ONLY
#include <gaia_spdlog/details/registry.h>
#endif

#include <gaia_spdlog/common.h>
#include <gaia_spdlog/details/periodic_worker.h>
#include <gaia_spdlog/logger.h>
#include <gaia_spdlog/pattern_formatter.h>

#ifndef GAIA_SPDLOG_DISABLE_DEFAULT_LOGGER
// support for the default stdout color logger
#ifdef _WIN32
#include <gaia_spdlog/sinks/wincolor_sink.h>
#else
#include <gaia_spdlog/sinks/ansicolor_sink.h>
#endif
#endif // GAIA_SPDLOG_DISABLE_DEFAULT_LOGGER

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace gaia_spdlog {
namespace details {

GAIA_SPDLOG_INLINE registry::registry()
    : formatter_(new pattern_formatter())
{

#ifndef GAIA_SPDLOG_DISABLE_DEFAULT_LOGGER
    // create default logger (ansicolor_stdout_sink_mt or wincolor_stdout_sink_mt in windows).
#ifdef _WIN32
    auto color_sink = std::make_shared<sinks::wincolor_stdout_sink_mt>();
#else
    auto color_sink = std::make_shared<sinks::ansicolor_stdout_sink_mt>();
#endif

    const char *default_logger_name = "";
    default_logger_ = std::make_shared<gaia_spdlog::logger>(default_logger_name, std::move(color_sink));
    loggers_[default_logger_name] = default_logger_;

#endif // GAIA_SPDLOG_DISABLE_DEFAULT_LOGGER
}

GAIA_SPDLOG_INLINE registry::~registry() = default;

GAIA_SPDLOG_INLINE void registry::register_logger(std::shared_ptr<logger> new_logger)
{
    std::lock_guard<std::mutex> lock(logger_map_mutex_);
    register_logger_(std::move(new_logger));
}

GAIA_SPDLOG_INLINE void registry::initialize_logger(std::shared_ptr<logger> new_logger)
{
    std::lock_guard<std::mutex> lock(logger_map_mutex_);
    new_logger->set_formatter(formatter_->clone());

    if (err_handler_)
    {
        new_logger->set_error_handler(err_handler_);
    }

    // set new level according to previously configured level or default level
    auto it = log_levels_.find(new_logger->name());
    auto new_level = it != log_levels_.end() ? it->second : global_log_level_;
    new_logger->set_level(new_level);

    new_logger->flush_on(flush_level_);

    if (backtrace_n_messages_ > 0)
    {
        new_logger->enable_backtrace(backtrace_n_messages_);
    }

    if (automatic_registration_)
    {
        register_logger_(std::move(new_logger));
    }
}

GAIA_SPDLOG_INLINE std::shared_ptr<logger> registry::get(const std::string &logger_name)
{
    std::lock_guard<std::mutex> lock(logger_map_mutex_);
    auto found = loggers_.find(logger_name);
    return found == loggers_.end() ? nullptr : found->second;
}

GAIA_SPDLOG_INLINE std::shared_ptr<logger> registry::default_logger()
{
    std::lock_guard<std::mutex> lock(logger_map_mutex_);
    return default_logger_;
}

// Return raw ptr to the default logger.
// To be used directly by the gaia_spdlog default api (e.g. gaia_spdlog::info)
// This make the default API faster, but cannot be used concurrently with set_default_logger().
// e.g do not call set_default_logger() from one thread while calling gaia_spdlog::info() from another.
GAIA_SPDLOG_INLINE logger *registry::get_default_raw()
{
    return default_logger_.get();
}

// set default logger.
// default logger is stored in default_logger_ (for faster retrieval) and in the loggers_ map.
GAIA_SPDLOG_INLINE void registry::set_default_logger(std::shared_ptr<logger> new_default_logger)
{
    std::lock_guard<std::mutex> lock(logger_map_mutex_);
    // remove previous default logger from the map
    if (default_logger_ != nullptr)
    {
        loggers_.erase(default_logger_->name());
    }
    if (new_default_logger != nullptr)
    {
        loggers_[new_default_logger->name()] = new_default_logger;
    }
    default_logger_ = std::move(new_default_logger);
}

GAIA_SPDLOG_INLINE void registry::set_tp(std::shared_ptr<thread_pool> tp)
{
    std::lock_guard<std::recursive_mutex> lock(tp_mutex_);
    tp_ = std::move(tp);
}

GAIA_SPDLOG_INLINE std::shared_ptr<thread_pool> registry::get_tp()
{
    std::lock_guard<std::recursive_mutex> lock(tp_mutex_);
    return tp_;
}

// Set global formatter. Each sink in each logger will get a clone of this object
GAIA_SPDLOG_INLINE void registry::set_formatter(std::unique_ptr<formatter> formatter)
{
    std::lock_guard<std::mutex> lock(logger_map_mutex_);
    formatter_ = std::move(formatter);
    for (auto &l : loggers_)
    {
        l.second->set_formatter(formatter_->clone());
    }
}

GAIA_SPDLOG_INLINE void registry::enable_backtrace(size_t n_messages)
{
    std::lock_guard<std::mutex> lock(logger_map_mutex_);
    backtrace_n_messages_ = n_messages;

    for (auto &l : loggers_)
    {
        l.second->enable_backtrace(n_messages);
    }
}

GAIA_SPDLOG_INLINE void registry::disable_backtrace()
{
    std::lock_guard<std::mutex> lock(logger_map_mutex_);
    backtrace_n_messages_ = 0;
    for (auto &l : loggers_)
    {
        l.second->disable_backtrace();
    }
}

GAIA_SPDLOG_INLINE void registry::set_level(level::level_enum log_level)
{
    std::lock_guard<std::mutex> lock(logger_map_mutex_);
    for (auto &l : loggers_)
    {
        l.second->set_level(log_level);
    }
    global_log_level_ = log_level;
}

GAIA_SPDLOG_INLINE void registry::flush_on(level::level_enum log_level)
{
    std::lock_guard<std::mutex> lock(logger_map_mutex_);
    for (auto &l : loggers_)
    {
        l.second->flush_on(log_level);
    }
    flush_level_ = log_level;
}

GAIA_SPDLOG_INLINE void registry::flush_every(std::chrono::seconds interval)
{
    std::lock_guard<std::mutex> lock(flusher_mutex_);
    auto clbk = [this]() { this->flush_all(); };
    periodic_flusher_ = details::make_unique<periodic_worker>(clbk, interval);
}

GAIA_SPDLOG_INLINE void registry::set_error_handler(void (*handler)(const std::string &msg))
{
    std::lock_guard<std::mutex> lock(logger_map_mutex_);
    for (auto &l : loggers_)
    {
        l.second->set_error_handler(handler);
    }
    err_handler_ = handler;
}

GAIA_SPDLOG_INLINE void registry::apply_all(const std::function<void(const std::shared_ptr<logger>)> &fun)
{
    std::lock_guard<std::mutex> lock(logger_map_mutex_);
    for (auto &l : loggers_)
    {
        fun(l.second);
    }
}

GAIA_SPDLOG_INLINE void registry::flush_all()
{
    std::lock_guard<std::mutex> lock(logger_map_mutex_);
    for (auto &l : loggers_)
    {
        l.second->flush();
    }
}

GAIA_SPDLOG_INLINE void registry::drop(const std::string &logger_name)
{
    std::lock_guard<std::mutex> lock(logger_map_mutex_);
    loggers_.erase(logger_name);
    if (default_logger_ && default_logger_->name() == logger_name)
    {
        default_logger_.reset();
    }
}

GAIA_SPDLOG_INLINE void registry::drop_all()
{
    std::lock_guard<std::mutex> lock(logger_map_mutex_);
    loggers_.clear();
    default_logger_.reset();
}

// clean all resources and threads started by the registry
GAIA_SPDLOG_INLINE void registry::shutdown()
{
    {
        std::lock_guard<std::mutex> lock(flusher_mutex_);
        periodic_flusher_.reset();
    }

    drop_all();

    {
        std::lock_guard<std::recursive_mutex> lock(tp_mutex_);
        tp_.reset();
    }
}

GAIA_SPDLOG_INLINE std::recursive_mutex &registry::tp_mutex()
{
    return tp_mutex_;
}

GAIA_SPDLOG_INLINE void registry::set_automatic_registration(bool automatic_registration)
{
    std::lock_guard<std::mutex> lock(logger_map_mutex_);
    automatic_registration_ = automatic_registration;
}

GAIA_SPDLOG_INLINE void registry::set_levels(log_levels levels, level::level_enum *global_level)
{
    std::lock_guard<std::mutex> lock(logger_map_mutex_);
    log_levels_ = std::move(levels);
    auto global_level_requested = global_level != nullptr;
    global_log_level_ = global_level_requested ? *global_level : global_log_level_;

    for (auto &logger : loggers_)
    {
        auto logger_entry = log_levels_.find(logger.first);
        if (logger_entry != log_levels_.end())
        {
            logger.second->set_level(logger_entry->second);
        }
        else if (global_level_requested)
        {
            logger.second->set_level(*global_level);
        }
    }
}

GAIA_SPDLOG_INLINE registry &registry::instance()
{
    static registry s_instance;
    return s_instance;
}

GAIA_SPDLOG_INLINE void registry::throw_if_exists_(const std::string &logger_name)
{
    if (loggers_.find(logger_name) != loggers_.end())
    {
        throw_spdlog_ex("logger with name '" + logger_name + "' already exists");
    }
}

GAIA_SPDLOG_INLINE void registry::register_logger_(std::shared_ptr<logger> new_logger)
{
    auto logger_name = new_logger->name();
    throw_if_exists_(logger_name);
    loggers_[logger_name] = std::move(new_logger);
}

} // namespace details
} // namespace gaia_spdlog
