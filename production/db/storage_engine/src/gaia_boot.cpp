/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_boot.hpp"

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
namespace db {

gaia_boot_t::gaia_boot_t() {
    unique_lock lock(m_lock);
    // Note, if directory doesn't exist, it must be created first.
    string boot_file_name(PERSISTENT_DIRECTORY_PATH);
    boot_file_name += "/boot_parameters.bin";
    while ((m_boot_file = fopen(boot_file_name.c_str(), "rb+")) == nullptr) {
        FILE* created_file = fopen(boot_file_name.c_str(), "w");
        if (created_file == nullptr) {
            fprintf(stderr, "I/O failure, cannot create %s, errno=%d\n", boot_file_name.c_str(), errno);
            throw_system_error("cannot create " + boot_file_name);
        }
    }
    size_t count = fread((char *)&m_boot_data, sizeof(m_boot_data), 1, m_boot_file);
    if (count == 0) {
        // The boot file didn't exist.
        m_boot_data.next_id = 0;
        m_boot_data.version = 0;
        m_boot_data.next_type = 0;
    }
}

gaia_boot_t::~gaia_boot_t() {
    save_gaia_boot();
    fclose(m_boot_file);
}

gaia_boot_t& gaia_boot_t::get() {
    static gaia_boot_t s_instance;
    return s_instance;
}

void gaia_boot_t::save_gaia_boot() {
    // Writing every time the boot record is updated will make sure the
    // contents are buffered in memory with thread safety.
    fseek(m_boot_file, 0, SEEK_SET);
    size_t count = fwrite((const char *)&m_boot_data, sizeof(m_boot_data), 1, m_boot_file);
    if (count == 0) {
        fprintf(stderr, "I/O failure %d, %ld bytes read\n", errno, count);
    }
}

gaia_id_t gaia_boot_t::get_next_id() {
    unique_lock lock(m_lock);
    ++m_boot_data.next_id;
    save_gaia_boot();
    return m_boot_data.next_id;
}

gaia_type_t gaia_boot_t::get_next_type() {
    unique_lock lock(m_lock);
    m_boot_data.next_type++;
    save_gaia_boot();
    return m_boot_data.next_type;
}

uint32_t gaia_boot_t::get_next_version() {
    unique_lock lock(m_lock);
    m_boot_data.version++;
    save_gaia_boot();
    return m_boot_data.version;
}

} // namespace db
} // namespace gaia
