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

namespace gaia {

namespace db {

using namespace common;

typedef uint64_t gaia_edge_type_t;

// 1K oughta be enough for anybody...
const size_t MAX_MSG_SIZE = 1 << 10;

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

    struct hash_node {
        gaia_id_t id;
        int64_t next;
        int64_t row_id;
    };

    struct data {
        // The first two fields are used as cross-process atomic counters.
        // We don't need something like a cross-process mutex for this,
        // as long as we use atomic intrinsics for mutating the counters.
        // This is because the instructions targeted by the intrinsics
        // operate at the level of physical memory, not virtual addresses.
        gaia_id_t next_id;
        int64_t row_id_count;
        int64_t hash_node_count;
        hash_node hash_nodes[HASH_BUCKETS + HASH_LIST_ELEMENTS];
        int64_t objects[MAX_RIDS * 8];
    };

    struct log {
        int64_t count;
        struct log_record {
            int64_t row_id;
            int64_t old_object;
            int64_t new_object;
        } log_records[MAX_LOG_RECS];
    };

    static int s_fd_offsets;
    static data* s_data;
    thread_local static log* s_log;
    thread_local static int s_session_socket;

   public:
    // The real implementation will need
    // to do something better than increment
    // a counter.  It will need to guarantee
    // that the generated id is not in use
    // already by a database that is
    // restored.
    static gaia_id_t generate_id() {
        gaia_id_t id = __sync_fetch_and_add(&s_data->next_id, 1);
        return id;
    }
};

}  // namespace db
}  // namespace gaia
