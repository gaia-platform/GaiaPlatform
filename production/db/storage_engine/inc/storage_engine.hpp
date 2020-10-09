/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/types.h>

#include <iostream>
#include <iomanip>
#include <cassert>
#include <set>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <string>
#include <sstream>

#include "scope_guard.hpp"
#include "system_error.hpp"
#include "gaia_common.hpp"
#include "gaia_db.hpp"
#include "db_types.hpp"
#include "gaia_boot.hpp"
#include "gaia_exception.hpp"
#include "retail_assert.hpp"

namespace gaia {
namespace db {

using namespace common;

// 1K oughta be enough for anybody...
constexpr size_t MAX_MSG_SIZE = 1 << 10;

enum class gaia_operation_t : uint8_t {
    create = 0x1,
    update = 0x2,
    remove = 0x3,
    clone = 0x4
};

struct hash_node {
    gaia_id_t id;
    size_t next_offset;
    gaia_locator_t locator;
};

class se_base {
    friend class gaia_ptr;
    friend class gaia_hash_map;

protected:
    static constexpr char SERVER_CONNECT_SOCKET_NAME[] = "gaia_se_server";
    static constexpr char SCH_MEM_LOCATORS[] = "gaia_mem_locators";
    static constexpr char SCH_MEM_DATA[] = "gaia_mem_data";
    static constexpr char SCH_MEM_LOG[] = "gaia_mem_log";

    static constexpr size_t MAX_LOCATORS = 32 * 128L * 1024L;
    static constexpr size_t HASH_BUCKETS = 12289;
    static constexpr size_t HASH_LIST_ELEMENTS = MAX_LOCATORS;
    static constexpr size_t MAX_LOG_RECS = 1000000;
    static constexpr size_t MAX_OBJECTS = MAX_LOCATORS * 8;

    typedef gaia_locator_t locators[MAX_LOCATORS];

    struct data {
        // The first two fields are used as cross-process atomic counters.
        // We don't need something like a cross-process mutex for this,
        // as long as we use atomic intrinsics for mutating the counters.
        // This is because the instructions targeted by the intrinsics
        // operate at the level of physical memory, not virtual addresses.
        gaia_id_t next_id;
        gaia_txn_id_t next_txn_id;
        size_t locator_count;
        size_t hash_node_count;
        hash_node hash_nodes[HASH_BUCKETS + HASH_LIST_ELEMENTS];
        // This array is actually an untyped array of bytes, but it's defined as
        // an array of uint64_t just to enforce 8-byte alignment. Allocating
        // (MAX_LOCATORS * 8) 8-byte words for this array means we reserve 64
        // bytes on average for each object we allocate (or 1 cache line on
        // every common architecture).
        uint64_t objects[MAX_LOCATORS * 8];
    };

    struct log {
        size_t count;
        struct log_record {
            gaia_locator_t locator;
            gaia_offset_t old_offset;
            gaia_offset_t new_offset;
            gaia_id_t deleted_id;
            gaia_operation_t operation;
        } log_records[MAX_LOG_RECS];
    };

    thread_local static log* s_log;
    thread_local static int s_session_socket;
    thread_local static gaia_txn_id_t s_txn_id;

public:
    static gaia_txn_id_t allocate_txn_id(data* s_data) {
        gaia_txn_id_t txn_id = __sync_add_and_fetch(&s_data->next_txn_id, 1);
        return txn_id;
    }

    static log* get_txn_log() {
        return s_log;
    }

    static inline gaia_locator_t allocate_locator(locators* locators, data* s_data, bool invoked_by_server = false) {
        if (invoked_by_server) {
            retail_assert(locators, "Server locators should be non-null");
        }

        if (locators == nullptr) {
            throw transaction_not_open();
        }

        if (s_data->locator_count >= MAX_LOCATORS) {
            throw oom();
        }

        return 1 + __sync_fetch_and_add(&s_data->locator_count, 1);
    }

    static void inline allocate_object(gaia_locator_t locator, size_t size, locators* locators, data* s_data, bool invoked_by_server = false) {
        if (invoked_by_server) {
            retail_assert(locators, "Server locators should be non-null");
        }

        if (locators == nullptr) {
            throw transaction_not_open();
        }

        if (s_data->objects[0] >= MAX_OBJECTS) {
            throw oom();
        }

        (*locators)[locator] = 1 + __sync_fetch_and_add(&s_data->objects[0], (size + sizeof(uint64_t) - 1) / sizeof(uint64_t));
    }

    static bool locator_exists(se_base::locators* locators, gaia_locator_t locator) {
        return (*locators)[locator];
    }
};

} // namespace db
} // namespace gaia
