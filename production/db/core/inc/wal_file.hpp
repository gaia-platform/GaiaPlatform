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

class wal_file_t
{
public:
    static inline uint64_t file_size{0};
    static inline wal_sequence_t file_num{0};
    static inline wal_file_offset_t current_offset{0};
    static inline std::string dir_name = "";
    static inline int dir_fd{0};
    static inline int file_fd{0};
    static inline std::string file_name = "";

    wal_file_t(std::string& dir_name, int dir_fd, wal_sequence_t file_num, size_t file_size);

    bool has_enough_space(size_t record_size);
    wal_file_offset_t get_current_offset();
    void allocate(size_t size);
    std::string get_file_name();
};

} // namespace db
} // namespace gaia
