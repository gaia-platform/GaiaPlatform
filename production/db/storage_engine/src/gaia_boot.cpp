/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_boot.hpp"

using namespace gaia::db;

namespace gaia {
namespace db {

// Assign successive integers for gaia_id_t, gaia_type_t and versions.
// The gaia_id_t (IDs) are expected to be high volume, and are persisted in blocks.
// If the block size is 1000, then each process will assign ID 1 to 1000,
// or 10001 to 11000, etc.
// Unused portions of a block (because process shut down) will remain forever
// unused. With a 64-bit space, there is no concern about running out.
gaia_boot_t::gaia_boot_t() {
    // Note, if directory doesn't exist, it must be created first.
    string boot_file_name(PERSISTENT_DIRECTORY_PATH);
    boot_file_name += "/boot_parameters.bin";
    while ((m_boot_file = fopen(boot_file_name.c_str(), "rb+")) == nullptr) {
        FILE* created_file = fopen(boot_file_name.c_str(), "w");
        if (created_file == nullptr) {
            // logger_t::db().critical("I/O failure, cannot create {}, error: {}\n", boot_file_name, strerror(errno));
            fprintf(stderr, "I/O failure, cannot create %s, error: %s\n", boot_file_name.c_str(), strerror(errno));
            throw_system_error("cannot create " + boot_file_name);
        }
        fclose(created_file);
        // Repeat the "rb+" open, which should succeed this time.
    }
    m_boot_fd = fileno(m_boot_file);
    flock(m_boot_fd, LOCK_EX);
    size_t count = fread((char*)&m_boot_data, sizeof(m_boot_data), 1, m_boot_file);
    if (count == 0) {
        // The boot file didn't exist.
        m_next_id = 1;
        m_boot_data.version = 1;
        m_boot_data.next_type = 1;
        m_boot_data.limit = c_block_delta;
        // logger_t::db().info("Created new boot record");
        fprintf(stderr, "Created new boot record\n");
    }
    else {
        m_boot_data.limit += c_block_delta;
        m_next_id = m_boot_data.limit - c_block_delta + 1;
        // logger_t::db().info("New ID block allocated, limit {}", m_boot_data.limit);
        fprintf(stderr, "New ID block allocated, limit %lu\n", m_boot_data.limit);
    }
    save_gaia_boot();
    flock(m_boot_fd, LOCK_UN);
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
    fseek(m_boot_file, 0, SEEK_SET);
    size_t count = fwrite((const char *)&m_boot_data, sizeof(m_boot_data), 1, m_boot_file);
    if (count == 0) {
        // logger_t::db().error("I/O failure '{}', {} bytes read", strerror(errno), count);
        fprintf(stderr, "I/O failure '%s', %zu bytes read\n", strerror(errno), count);
        throw_system_error("cannot write to system boot record");
    }
}

// Request the next available object ID.
// Permit rapid assignment of c_block_delta IDs, then request a new block.
gaia_id_t gaia_boot_t::get_next_id() {
    gaia_id_t next_id = __sync_fetch_and_add(&m_next_id, 1);
    // Upon use of the block of IDs, one thread will request another block.
    if (next_id == m_boot_data.limit) {
        flock(m_boot_fd, LOCK_EX);
        fread((char*)&m_boot_data, sizeof(m_boot_data), 1, m_boot_file);
        // Note that the limit may have been bumped by other processes.
        m_boot_data.limit += c_block_delta;
        save_gaia_boot();
        // First id of the next block.
        m_next_id = m_boot_data.limit - c_block_delta + 1;
        // logger_t::db().info("New ID block allocated, limit %lu", m_boot_data.limit);
fprintf(stderr, "**LIMIT %lu** ", m_boot_data.limit);
        flock(m_boot_fd, LOCK_UN);
    }
    // If other threads request an ID after the block is allocated, they must
    // wait for the first thread to complete the block request.
    else if (next_id > m_boot_data.limit) {
fprintf(stderr, "**SECOND THREAD** ");
        flock(m_boot_fd, LOCK_EX);
        // If this thread beat the thread that updates the boot record, it will have
        // to wait until the update is accomplished.
        while (next_id > m_boot_data.limit) {
            flock(m_boot_fd, LOCK_UN);
            flock(m_boot_fd, LOCK_EX);
        }
        flock(m_boot_fd, LOCK_UN);
        next_id = __sync_fetch_and_add(&m_next_id, 1);
    }
fprintf(stderr, "%ld ", next_id);
    return next_id;
}

// Request the next available container type.
// As a less-requested counter, this updates the boot record immediately.
gaia_type_t gaia_boot_t::get_next_type() {
    flock(m_boot_fd, LOCK_EX);
    gaia_type_t type = m_boot_data.next_type++;
    save_gaia_boot();
    flock(m_boot_fd, LOCK_UN);
    return type;
}

// Versions are not yet implemented. Give this a legitimate type if/when that day comes.
uint32_t gaia_boot_t::get_next_version() {
    flock(m_boot_fd, LOCK_EX);
    uint32_t version = m_boot_data.version++;
    save_gaia_boot();
    flock(m_boot_fd, LOCK_UN);
    return version;
}

} // namespace db
} // namespace gaia
