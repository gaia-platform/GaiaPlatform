/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <csignal>
#include <thread>
#include <atomic>
#include <unordered_set>

#include <flatbuffers/flatbuffers.h>

#include "array_size.hpp"
#include "retail_assert.hpp"
#include "system_error.hpp"
#include "mmap_helpers.hpp"
#include "socket_helpers.hpp"
#include "messages_generated.h"
#include "storage_engine.hpp"
#include "triggers.hpp"
#include "event_trigger_threadpool.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db::triggers;

namespace gaia {

namespace db {

// We need to forward-declare this class to avoid a circular dependency.
class gaia_hash_map;

class client : private se_base {
    friend class gaia_ptr;
    friend class gaia_hash_map;
    friend class event_trigger_threadpool_t;

   public:
    static inline bool is_transaction_active() {
        return (*s_offsets != nullptr);
    }
    static void begin_session();
    static void end_session();
    static void begin_transaction();
    static void rollback_transaction();
    static void commit_transaction();

    // This is test-only functionality, intended to be exposed only in internal headers.
    static void clear_shared_memory();

   private:
    thread_local static int s_fd_log;
    thread_local static offsets* s_offsets;
    thread_local static std::vector<gaia::db::triggers::trigger_event_t> s_events;

    // Maintain a static filter in the client to disable generating events
    // for system types.
    static std::unordered_set<gaia_type_t> trigger_excluded_types;

    // Threadpool to help invoke post-commit triggers in response to events generated in each transaction.
    static gaia::db::triggers::event_trigger_threadpool_t* event_trigger_pool;

    // Inherited from se_base:
    // static int s_fd_offsets;
    // static data *s_data;
    // thread_local static log *s_log;
    // thread_local static gaia_xid_t s_transaction_id;

    static void tx_cleanup();

    static void destroy_log_mapping();

    static int get_session_socket();

    /**
     * Function returns whether to generate a trigger event for an operation based on the gaia_type_t.
     */
    static inline bool is_invalid_event(const gaia_type_t type) {
        return trigger_excluded_types.find(type) != trigger_excluded_types.end();
    }

    static inline int64_t allocate_row_id() {
        if (*s_offsets == nullptr) {
            throw transaction_not_open();
        }

        if (s_data->row_id_count >= MAX_RIDS) {
            throw oom();
        }

        return 1 + __sync_fetch_and_add(&s_data->row_id_count, 1);
    }

    static void inline allocate_object(int64_t row_id, size_t size) {
        if (*s_offsets == nullptr) {
            throw transaction_not_open();
        }

        if (s_data->objects[0] >= MAX_OBJECTS) {
            throw oom();
        }

        (*s_offsets)[row_id] = 1 + __sync_fetch_and_add(
            &s_data->objects[0],
            (size + sizeof(int64_t) - 1) / sizeof(int64_t));
    }

    static inline void verify_tx_active() {
        if (!is_transaction_active()) {
            throw transaction_not_open();
        }
    }

    static inline void verify_no_tx() {
        if (is_transaction_active()) {
            throw transaction_in_progress();
        }
    }

    static inline void verify_session_active() {
        if (s_session_socket == -1) {
            throw no_session_active();
        }
    }

    static inline void verify_no_session() {
        if (s_session_socket != -1) {
            throw session_exists();
        }
    }

    static inline void tx_log(
        int64_t row_id,
        int64_t old_object,
        int64_t new_object,
        // 'id' & 'type' are required to keep track of deleted keys which will be propagated to the persistent layer.
        // Drawback of this approach is that we're keeping 8 + 8 bytes of memory (for other operations) in the transient log.
        // One approach would be to introduce a separate log for deleted keys; 
        // another alternative would be to keep the key as <id> only (as opposed to <type, id>)
        gaia_operation_t operation,
        gaia_id_t id = 0,
        gaia_type_t type = 0) {
        retail_assert(s_log->count < MAX_LOG_RECS);

        log::log_record* lr = s_log->log_records + s_log->count++;

        lr->row_id = row_id;
        lr->old_object = old_object;
        lr->new_object = new_object;
        lr->id = id;
        lr->type = type;
        lr->operation = operation;
    }
};

}  // namespace db
}  // namespace gaia
