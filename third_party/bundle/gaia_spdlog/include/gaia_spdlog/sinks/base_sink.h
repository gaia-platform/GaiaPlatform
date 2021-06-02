// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once
//
// base sink templated over a mutex (either dummy or real)
// concrete implementation should override the sink_it_() and flush_()  methods.
// locking is taken care of in this class - no locking needed by the
// implementers..
//

#include <gaia_spdlog/common.h>
#include <gaia_spdlog/details/log_msg.h>
#include <gaia_spdlog/sinks/sink.h>

namespace gaia_spdlog {
namespace sinks {
template<typename Mutex>
class base_sink : public sink
{
public:
    base_sink();
    explicit base_sink(std::unique_ptr<gaia_spdlog::formatter> formatter);
    ~base_sink() override = default;

    base_sink(const base_sink &) = delete;
    base_sink(base_sink &&) = delete;

    base_sink &operator=(const base_sink &) = delete;
    base_sink &operator=(base_sink &&) = delete;

    void log(const details::log_msg &msg) final;
    void flush() final;
    void set_pattern(const std::string &pattern) final;
    void set_formatter(std::unique_ptr<gaia_spdlog::formatter> sink_formatter) final;

protected:
    // sink formatter
    std::unique_ptr<gaia_spdlog::formatter> formatter_;
    Mutex mutex_;

    virtual void sink_it_(const details::log_msg &msg) = 0;
    virtual void flush_() = 0;
    virtual void set_pattern_(const std::string &pattern);
    virtual void set_formatter_(std::unique_ptr<gaia_spdlog::formatter> sink_formatter);
};
} // namespace sinks
} // namespace gaia_spdlog

#ifdef GAIA_SPDLOG_HEADER_ONLY
#include "base_sink-inl.h"
#endif
