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
    log_file_t(const std::string& dir_name, int dir_fd, log_sequence_t log_seq, size_t file_size);

    /**
     * Obtain offset to write the next log record at.
     */
    file_offset_t get_current_offset();

    /**
     * Get remaining space in persistent log file.
     */
    size_t get_bytes_remaining_after_append(size_t record_size);

    /**
     * Allocate space in persistent log file.
     */
    void allocate(size_t size);

    int get_file_fd();

    /**
     * Obtain sequence number for the file.
     */
    log_sequence_t get_log_sequence();

private:
    size_t m_file_size;
    log_sequence_t m_log_seq;
    file_offset_t m_current_offset;
    std::string m_dir_name;
    int m_dir_fd;
    int m_file_fd;
    std::string m_file_name;
    inline static constexpr int c_file_permissions = 0666;

    // We reserve space for a header at the end of a log file.
    // This serves to signify the end of a file.
    inline static constexpr size_t c_reserved_size = sizeof(record_header_t);
};

} // namespace persistence
} // namespace db
} // namespace gaia
