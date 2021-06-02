// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <gaia_spdlog/details/log_msg_buffer.h>
#include <gaia_spdlog/details/circular_q.h>

#include <atomic>
#include <mutex>
#include <functional>

// Store log messages in circular buffer.
// Useful for storing debug data in case of error/warning happens.

namespace gaia_spdlog {
namespace details {
class GAIA_SPDLOG_API backtracer
{
    mutable std::mutex mutex_;
    std::atomic<bool> enabled_{false};
    circular_q<log_msg_buffer> messages_;

public:
    backtracer() = default;
    backtracer(const backtracer &other);

    backtracer(backtracer &&other) GAIA_SPDLOG_NOEXCEPT;
    backtracer &operator=(backtracer other);

    void enable(size_t size);
    void disable();
    bool enabled() const;
    void push_back(const log_msg &msg);

    // pop all items in the q and apply the given fun on each of them.
    void foreach_pop(std::function<void(const details::log_msg &)> fun);
};

} // namespace details
} // namespace gaia_spdlog

#ifdef GAIA_SPDLOG_HEADER_ONLY
#include "backtracer-inl.h"
#endif
