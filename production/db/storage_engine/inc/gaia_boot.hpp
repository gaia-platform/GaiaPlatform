/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <shared_mutex>
#include <stdio.h>

#include "gaia_ptr.hpp"
#include "system_error.hpp"
#include "gaia_db_internal.hpp"

namespace gaia {
namespace db {

class gaia_boot_t {
public:
    gaia_boot_t(gaia_boot_t&) = delete;
    void operator=(gaia_boot_t const&) = delete;
    static gaia_boot_t& get();

    gaia_id_t get_next_id();
    gaia_type_t get_next_type();
    uint32_t get_next_version();
    void save_gaia_boot();
private:
    gaia_boot_t();
    ~gaia_boot_t();
    FILE* m_boot_file;
    struct {
        gaia_id_t next_id;
        uint32_t version;
        gaia_type_t next_type;
    } m_boot_data;
    mutable shared_mutex m_lock;
};

} // namespace db
} // namespace gaia
