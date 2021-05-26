// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <gaia_spdlog/details/null_mutex.h>
#include <gaia_spdlog/sinks/base_sink.h>
#include <gaia_spdlog/details/synchronous_factory.h>

#include <mutex>

namespace gaia_spdlog {
namespace sinks {

template<typename Mutex>
class null_sink : public base_sink<Mutex>
{
protected:
    void sink_it_(const details::log_msg &) override {}
    void flush_() override {}
};

using null_sink_mt = null_sink<details::null_mutex>;
using null_sink_st = null_sink<details::null_mutex>;

} // namespace sinks

template<typename Factory = gaia_spdlog::synchronous_factory>
inline std::shared_ptr<logger> null_logger_mt(const std::string &logger_name)
{
    auto null_logger = Factory::template create<sinks::null_sink_mt>(logger_name);
    null_logger->set_level(level::off);
    return null_logger;
}

template<typename Factory = gaia_spdlog::synchronous_factory>
inline std::shared_ptr<logger> null_logger_st(const std::string &logger_name)
{
    auto null_logger = Factory::template create<sinks::null_sink_st>(logger_name);
    null_logger->set_level(level::off);
    return null_logger;
}

} // namespace gaia_spdlog
