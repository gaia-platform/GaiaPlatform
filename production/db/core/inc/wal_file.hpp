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

namespace gaia
{
namespace db
{

typedef uint32_t wal_sequence_t;
typedef uint32_t wal_file_offset_t;

class wal_file_t
{
public:
    static uint64_t file_size;
    static wal_sequence_t file_num;
    static wal_file_offset_t current_offset;
    static std::string dir_name;
    static int dir_fd;
    static int file_fd;

    wal_file_t(std::string& dir_name, int dir_fd, wal_sequence_t file_num, size_t file_size);
    ~wal_file_t();

    bool has_enough_space(size_t record_size);
    wal_file_offset_t get_current_offset();
    void allocate(size_t size);

private:
    void validate_record_size(size_t record_size);
};

} // namespace db
} // namespace gaia
