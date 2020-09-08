/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <shared_mutex>

#include "gaia_catalog.hpp"
#include "gaia_ptr.hpp"

constexpr gaia_type_t c_system_base_type = 0;
constexpr gaia_type_t c_system_base_id = 1;

namespace gaia {
namespace catalog {

class system_base_t {
public:
    system_base_t(system_base_t&) = delete;
    void operator=(system_base_t const&) = delete;
    static system_base_t& get();

    void init_system_base();
    gaia_type_t get_next_type();
    uint32_t get_next_version();
    void save_system_base();
private:
    system_base_t();
    ~system_base_t() {}
    struct {
        uint32_t version;
        gaia_type_t next_type;
    } m_base_data;
    mutable shared_mutex m_lock;
};

} // namespace catalog
} // namespace gaia
