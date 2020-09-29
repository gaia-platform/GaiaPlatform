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
#include "gaia_exception.hpp"
#include "gaia_record.hpp"
#include "retail_assert.hpp"

namespace gaia {
namespace db {

using namespace common;

typedef uint64_t gaia_edge_type_t;

// 1K oughta be enough for anybody...
const size_t MAX_MSG_SIZE = 1 << 10;

enum class gaia_operation_t: uint8_t {
    create = 0x1,
    update = 0x2,
    remove = 0x3,
    clone  = 0x4
};

struct hash_node {
    gaia_id_t id;
    int64_t next;
    int64_t row_id;
};

class se_base {
    friend class gaia_ptr;
    friend class gaia_hash_map;

   protected:
    static const char* const SERVER_CONNECT_SOCKET_NAME;
    static const char* const SCH_MEM_OFFSETS;
    static const char* const SCH_MEM_DATA;
    static const char* const SCH_MEM_LOG;

    auto static const MAX_RIDS = 32 * 128L * 1024L;
    static const auto HASH_BUCKETS = 12289;
    static const auto HASH_LIST_ELEMENTS = MAX_RIDS;
    static const auto MAX_LOG_RECS = 1000000;
    static const auto MAX_OBJECTS = MAX_RIDS * 8;

    typedef int64_t offsets[MAX_RIDS];

    struct data {
        // The first two fields are used as cross-process atomic counters.
        // We don't need something like a cross-process mutex for this,
        // as long as we use atomic intrinsics for mutating the counters.
        // This is because the instructions targeted by the intrinsics
        // operate at the level of physical memory, not virtual addresses.
        gaia_id_t next_id;
        int64_t transaction_id_count;
        int64_t row_id_count;
        int64_t hash_node_count;
        hash_node hash_nodes[HASH_BUCKETS + HASH_LIST_ELEMENTS];
        int64_t objects[MAX_RIDS * 8];
    };

    struct log {
        size_t count;
        struct log_record {
            int64_t row_id;
            int64_t old_object;
            int64_t new_object;
            gaia_id_t deleted_id;
            gaia_operation_t operation;
        } log_records[MAX_LOG_RECS];
    };

    thread_local static log* s_log;
    thread_local static int s_session_socket;
    thread_local static gaia_xid_t s_transaction_id;

   public:
    // The real implementation will need
    // to do something better than increment
    // a counter.  It will need to guarantee
    // that the generated id is not in use
    // already by a database that is
    // restored.
    static gaia_id_t generate_id(data* s_data) {
        gaia_id_t id = __sync_add_and_fetch(&s_data->next_id, 1);
        return id;
    }

    static gaia_xid_t allocate_transaction_id(data* s_data) {
        gaia_xid_t xid = __sync_add_and_fetch (&s_data->transaction_id_count, 1);
        return xid;
    }

    static log* get_txn_log() {
        return s_log;
    }

    static inline int64_t allocate_row_id(offsets* offsets, data* s_data, bool invoked_by_server = false) {
        if (invoked_by_server) {
            retail_assert(offsets, "Server offsets should be non-null");
        }

        if (offsets == nullptr) {
            throw transaction_not_open();
        }

        if (s_data->row_id_count >= MAX_RIDS) {
            throw oom();
        }

        return 1 + __sync_fetch_and_add(&s_data->row_id_count, 1);
    }

    static void inline allocate_object(int64_t row_id, uint64_t size, offsets* offsets, data* s_data, bool invoked_by_server = false) {
        if (invoked_by_server) {
            retail_assert(offsets, "Server offsets should be non-null");
        }

        if (offsets == nullptr) {
            throw transaction_not_open();
        }

        if (s_data->objects[0] >= MAX_OBJECTS) {
            throw oom();
        }

        (*offsets)[row_id] = 1 + __sync_fetch_and_add(
            &s_data->objects[0],
            (size + sizeof(int64_t) - 1) / sizeof(int64_t));
    }

    static bool locator_exists(se_base::offsets* offsets, int64_t offset) {
        return (*offsets)[offset];
    }
};

}  // namespace db
}  // namespace gaia
