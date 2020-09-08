/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "system_base.hpp"

using namespace gaia::db;

// System_base_t is a singleton object that manages the very first object
// in the storage engine. It has ID 1 with type 0. It has only one purpose now,
// which is to allocate gaia_type_t values (32-bit values, starting at 1).
//
// A version number has been included in this managed data to allow detection
// of the version of the database, which will be helpful to interpret its
// format.
//
// System_base_t assumes it is used within a transaction.
//

namespace gaia {
namespace catalog {

void system_base_t::init_system_base() {
    auto node_ptr = gaia_ptr::open(c_system_base_id);
    if (!node_ptr) {
        node_ptr = gaia_ptr::create(c_system_base_id, c_system_base_type, 0, sizeof(m_base_data), &m_base_data);
    }
    else {
        memcpy(&m_base_data, node_ptr.data(), node_ptr.data_size());
    }
}

system_base_t::system_base_t() {
    unique_lock lock(m_lock);
    m_base_data.version = 0;
    m_base_data.next_type = 0;
    init_system_base();
}

system_base_t& system_base_t::get() {
    static system_base_t s_instance;
    return s_instance;
}

void system_base_t::save_system_base() {
    auto node_ptr = gaia_ptr::open(c_system_base_id);
    node_ptr.update_payload(sizeof(m_base_data), &m_base_data);
}

gaia_type_t system_base_t::get_next_type() {
    unique_lock lock(m_lock);
    init_system_base();
    m_base_data.next_type++;
    get().save_system_base();
    return m_base_data.next_type;
}

uint32_t system_base_t::get_next_version() {
    unique_lock lock(m_lock);
    init_system_base();
    m_base_data.version++;
    save_system_base();
    return m_base_data.version;
}

} // namespace catalog
} // namespace gaia
