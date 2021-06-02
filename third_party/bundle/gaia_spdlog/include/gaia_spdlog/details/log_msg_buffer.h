// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <gaia_spdlog/details/log_msg.h>

namespace gaia_spdlog {
namespace details {

// Extend log_msg with internal buffer to store its payload.
// This is needed since log_msg holds string_views that points to stack data.

class GAIA_SPDLOG_API log_msg_buffer : public log_msg
{
    memory_buf_t buffer;
    void update_string_views();

public:
    log_msg_buffer() = default;
    explicit log_msg_buffer(const log_msg &orig_msg);
    log_msg_buffer(const log_msg_buffer &other);
    log_msg_buffer(log_msg_buffer &&other) GAIA_SPDLOG_NOEXCEPT;
    log_msg_buffer &operator=(const log_msg_buffer &other);
    log_msg_buffer &operator=(log_msg_buffer &&other) GAIA_SPDLOG_NOEXCEPT;
};

} // namespace details
} // namespace gaia_spdlog

#ifdef GAIA_SPDLOG_HEADER_ONLY
#include "log_msg_buffer-inl.h"
#endif
