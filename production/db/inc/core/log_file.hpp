/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <fcntl.h>
#include <unistd.h>

#include <cstddef>

#include <atomic>
#include <memory>
#include <string>

#include "persistence_types.hpp"

namespace gaia
{
namespace db
{
namespace persistence
{

/**
 * Layer for persistent log file management.
 */
class log_file_t
{
public:
    log_file_t(const std::string& dir_name, int dir_fd, file_sequence_t file_seq, size_t file_size);

    /**
     * Obtain offset to write the next log record at.
     */
    file_offset_t get_current_offset() const;

    /**
     * Get remaining space in persistent log file.
     */
    size_t get_bytes_remaining_after_append(size_t record_size) const;

    /**
     * Allocate space in persistent log file.
     */
    void allocate(size_t size);

    int get_file_fd() const;

    /**
     * Obtain sequence number for the file.
     */
    file_sequence_t get_file_sequence() const;

private:
    size_t m_file_size{0};
    file_sequence_t m_file_seq{0};
    size_t m_current_offset{0};
    std::string m_dir_name{};
    int m_dir_fd{-1};
    int m_file_fd{-1};
    std::string m_file_name{};
    inline static constexpr int c_file_permissions{0666};
};

} // namespace persistence
} // namespace db
} // namespace gaia
