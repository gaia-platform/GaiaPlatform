/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "gaia_catalog.hpp"
#include "gaia_ptr.hpp"

constexpr gaia_type_t c_system_base_type = 0;
constexpr gaia_type_t c_system_base_id = 1;

class system_base_t {
    struct {
        gaia_type_t next_type;
        uint32_t version;
    } m_base_data;
    void save_system_base();
public:
    void init_system_base();
    system_base_t() : m_base_data{0,0} {}
    gaia_type_t get_next_type();
    uint32_t get_next_version();
};
