// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#ifndef GAIA_SPDLOG_HEADER_ONLY
#include <gaia_spdlog/sinks/basic_file_sink.h>
#endif

#include <gaia_spdlog/common.h>
#include <gaia_spdlog/details/os.h>

namespace gaia_spdlog {
namespace sinks {

template<typename Mutex>
GAIA_SPDLOG_INLINE basic_file_sink<Mutex>::basic_file_sink(const filename_t &filename, bool truncate)
{
    file_helper_.open(filename, truncate);
}

template<typename Mutex>
GAIA_SPDLOG_INLINE const filename_t &basic_file_sink<Mutex>::filename() const
{
    return file_helper_.filename();
}

template<typename Mutex>
GAIA_SPDLOG_INLINE void basic_file_sink<Mutex>::sink_it_(const details::log_msg &msg)
{
    memory_buf_t formatted;
    base_sink<Mutex>::formatter_->format(msg, formatted);
    file_helper_.write(formatted);
}

template<typename Mutex>
GAIA_SPDLOG_INLINE void basic_file_sink<Mutex>::flush_()
{
    file_helper_.flush();
}

} // namespace sinks
} // namespace gaia_spdlog
