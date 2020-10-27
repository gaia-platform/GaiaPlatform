/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>

#include "gaia_db_internal.hpp"
#include "gaia_ptr.hpp"
// #include "logger.hpp"
#include "system_error.hpp"

namespace gaia
{
namespace db
{

class gaia_boot_t
{
public:
    // Permissions on boot file.
    static constexpr mode_t c_rw_rw_rw = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    gaia_boot_t(gaia_boot_t&) = delete;
    void operator=(gaia_boot_t const&) = delete;
    static gaia_boot_t& get();

    gaia_id_t get_next_id();
    gaia_type_t get_next_type();
    uint32_t get_next_version();
    void reset_gaia_boot();
    void open_gaia_boot();

private:
    gaia_boot_t();
    ~gaia_boot_t();
    void save_gaia_boot();
    static constexpr int32_t c_block_delta = 1000;
    int m_boot_fd;
    // Persisted values.
    struct
    {
        gaia_id_t limit;
        gaia_type_t next_type;
        uint32_t version;
    } m_boot_data;
    // The next ID doesn't need to be persisted because it always starts out
    // at the beginning of a new block.
    gaia_id_t m_next_id;
};

} // namespace db
} // namespace gaia
