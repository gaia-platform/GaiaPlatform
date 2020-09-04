/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "system_base.hpp"

using namespace gaia::db;

void system_base_t::init_system_base() {
    auto node_ptr = gaia_ptr::open(c_system_base_id);
    if (!node_ptr) {
        node_ptr = gaia_ptr::create(c_system_base_id, c_system_base_type, 0, sizeof(m_base_data), &m_base_data);
    }
    else {
        memcpy(&m_base_data, node_ptr.data(), node_ptr.data_size());
    }
}

void system_base_t::save_system_base() {
     auto node_ptr = gaia_ptr::open(c_system_base_id);
     node_ptr.update_payload(sizeof(m_base_data), &m_base_data);
}

gaia_type_t system_base_t::get_next_type() {
    init_system_base();
    m_base_data.next_type++;
    save_system_base();
    return m_base_data.next_type;
}

uint32_t system_base_t::get_next_version() {
    init_system_base();
    m_base_data.version++;
    save_system_base();
    return m_base_data.version;
}
