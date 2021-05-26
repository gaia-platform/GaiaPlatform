// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <gaia_spdlog/details/file_helper.h>
#include <gaia_spdlog/details/null_mutex.h>
#include <gaia_spdlog/sinks/base_sink.h>
#include <gaia_spdlog/details/synchronous_factory.h>

#include <mutex>
#include <string>

namespace gaia_spdlog {
namespace sinks {
/*
 * Trivial file sink with single file as target
 */
template<typename Mutex>
class basic_file_sink final : public base_sink<Mutex>
{
public:
    explicit basic_file_sink(const filename_t &filename, bool truncate = false);
    const filename_t &filename() const;

protected:
    void sink_it_(const details::log_msg &msg) override;
    void flush_() override;

private:
    details::file_helper file_helper_;
};

using basic_file_sink_mt = basic_file_sink<std::mutex>;
using basic_file_sink_st = basic_file_sink<details::null_mutex>;

} // namespace sinks

//
// factory functions
//
template<typename Factory = gaia_spdlog::synchronous_factory>
inline std::shared_ptr<logger> basic_logger_mt(const std::string &logger_name, const filename_t &filename, bool truncate = false)
{
    return Factory::template create<sinks::basic_file_sink_mt>(logger_name, filename, truncate);
}

template<typename Factory = gaia_spdlog::synchronous_factory>
inline std::shared_ptr<logger> basic_logger_st(const std::string &logger_name, const filename_t &filename, bool truncate = false)
{
    return Factory::template create<sinks::basic_file_sink_st>(logger_name, filename, truncate);
}

} // namespace gaia_spdlog

#ifdef GAIA_SPDLOG_HEADER_ONLY
#include "basic_file_sink-inl.h"
#endif
