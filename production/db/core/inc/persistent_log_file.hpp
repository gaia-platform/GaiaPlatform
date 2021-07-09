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

/**
 * Layer for persistent log file management.
 */
class persistent_log_file_t
{
public:
    persistent_log_file_t(const std::string& dir_name, int dir_fd, size_t file_num, size_t file_size);

    /**
     * Obtain offset to write the next log record at.
     */
    size_t get_current_offset();

    /**
     * Get remaining space in persistent log file.
     */
    size_t get_remaining_space(size_t record_size);

    /**
     * Allocate space in persistent log file.
     */
    void allocate(size_t size);

    /**
     * Just for testing.
     */
    void get_file_name(std::string& file_name);

    int get_file_fd();

private:
    size_t m_file_size;
    size_t m_file_num;
    size_t m_current_offset;
    std::string m_dir_name;
    int m_dir_fd;
    int m_file_fd;
    std::string m_file_name;
    static constexpr int c_file_permissions = 0666;
};

} // namespace db
} // namespace gaia
