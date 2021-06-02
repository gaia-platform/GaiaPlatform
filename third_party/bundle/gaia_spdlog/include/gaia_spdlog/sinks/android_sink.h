// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#ifdef __ANDROID__

#include <gaia_spdlog/details/fmt_helper.h>
#include <gaia_spdlog/details/null_mutex.h>
#include <gaia_spdlog/details/os.h>
#include <gaia_spdlog/sinks/base_sink.h>
#include <gaia_spdlog/details/synchronous_factory.h>

#include <android/log.h>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>

#if !defined(GAIA_SPDLOG_ANDROID_RETRIES)
#define GAIA_SPDLOG_ANDROID_RETRIES 2
#endif

namespace gaia_spdlog {
namespace sinks {

/*
 * Android sink (logging using __android_log_write)
 */
template<typename Mutex>
class android_sink final : public base_sink<Mutex>
{
public:
    explicit android_sink(std::string tag = "gaia_spdlog", bool use_raw_msg = false)
        : tag_(std::move(tag))
        , use_raw_msg_(use_raw_msg)
    {}

protected:
    void sink_it_(const details::log_msg &msg) override
    {
        const android_LogPriority priority = convert_to_android_(msg.level);
        memory_buf_t formatted;
        if (use_raw_msg_)
        {
            details::gaia_fmt_helper::append_string_view(msg.payload, formatted);
        }
        else
        {
            base_sink<Mutex>::formatter_->format(msg, formatted);
        }
        formatted.push_back('\0');
        const char *msg_output = formatted.data();

        // See system/core/liblog/logger_write.c for explanation of return value
        int ret = __android_log_write(priority, tag_.c_str(), msg_output);
        int retry_count = 0;
        while ((ret == -11 /*EAGAIN*/) && (retry_count < GAIA_SPDLOG_ANDROID_RETRIES))
        {
            details::os::sleep_for_millis(5);
            ret = __android_log_write(priority, tag_.c_str(), msg_output);
            retry_count++;
        }

        if (ret < 0)
        {
            throw_spdlog_ex("__android_log_write() failed", ret);
        }
    }

    void flush_() override {}

private:
    static android_LogPriority convert_to_android_(gaia_spdlog::level::level_enum level)
    {
        switch (level)
        {
        case gaia_spdlog::level::trace:
            return ANDROID_LOG_VERBOSE;
        case gaia_spdlog::level::debug:
            return ANDROID_LOG_DEBUG;
        case gaia_spdlog::level::info:
            return ANDROID_LOG_INFO;
        case gaia_spdlog::level::warn:
            return ANDROID_LOG_WARN;
        case gaia_spdlog::level::err:
            return ANDROID_LOG_ERROR;
        case gaia_spdlog::level::critical:
            return ANDROID_LOG_FATAL;
        default:
            return ANDROID_LOG_DEFAULT;
        }
    }

    std::string tag_;
    bool use_raw_msg_;
};

using android_sink_mt = android_sink<std::mutex>;
using android_sink_st = android_sink<details::null_mutex>;
} // namespace sinks

// Create and register android syslog logger

template<typename Factory = gaia_spdlog::synchronous_factory>
inline std::shared_ptr<logger> android_logger_mt(const std::string &logger_name, const std::string &tag = "gaia_spdlog")
{
    return Factory::template create<sinks::android_sink_mt>(logger_name, tag);
}

template<typename Factory = gaia_spdlog::synchronous_factory>
inline std::shared_ptr<logger> android_logger_st(const std::string &logger_name, const std::string &tag = "gaia_spdlog")
{
    return Factory::template create<sinks::android_sink_st>(logger_name, tag);
}

} // namespace gaia_spdlog

#endif // __ANDROID__
