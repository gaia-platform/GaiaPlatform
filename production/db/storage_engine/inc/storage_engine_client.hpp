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

#include "retail_assert.hpp"
#include "system_error.hpp"
#include "scope_guard.hpp"
#include "mmap_helpers.hpp"
#include "socket_helpers.hpp"
#include "messages_generated.h"
#include "storage_engine.hpp"
#include "triggers.hpp"
#include "gaia_db_internal.hpp"

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
        return (s_locators != nullptr);
    }
    static void begin_session();
    static void end_session();
    static void begin_transaction();
    static void rollback_transaction();
    static void commit_transaction();

    // This is test-only functionality, intended to be exposed only in internal headers.
    static void clear_shared_memory();

   private:
    // Both s_fd_log & s_locators have transaction lifetime.
    thread_local static int s_fd_log;
    thread_local static locators* s_locators;
    // Both s_fd_locators & s_data have session lifetime.
    thread_local static int s_fd_locators;
    thread_local static data* s_data;
    // s_events has transaction lifetime and is cleared after each transaction.
    thread_local static std::vector<gaia::db::triggers::trigger_event_t> s_events;

    // Maintain a static filter in the client to disable generating events
    // for system types.
    static std::unordered_set<gaia_type_t> trigger_excluded_types;

    // Inherited from se_base:
    // thread_local static log *s_log;
    // thread_local static gaia_txn_id_t s_transaction_id;

    static void txn_cleanup();

    static void destroy_log_mapping();

    static int get_session_socket();

    /**
     *  Check if an event should be generated for a given type.
     */
    static inline bool is_valid_event(gaia_type_t type) {
        return (gaia::db::s_txn_commit_trigger
            && (trigger_excluded_types.find(type) == trigger_excluded_types.end()));
    }

    static inline void verify_txn_active() {
        if (!is_transaction_active()) {
            throw transaction_not_open();
        }
    }

    static inline void verify_no_txn() {
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

    static inline void txn_log(
        gaia_locator_t locator,
        gaia_offset_t old_offset,
        gaia_offset_t new_offset,
        gaia_operation_t operation,
        // 'deleted_id' is required to keep track of deleted keys which will be propagated to the persistent layer.
        // Memory for other operations will be unused. An alternative would be to keep a separate log for deleted keys only.
        gaia_id_t deleted_id = 0) {
        retail_assert(s_log->count < MAX_LOG_RECS);
        log::log_record* lr = s_log->log_records + s_log->count++;
        lr->locator = locator;
        lr->old_offset = old_offset;
        lr->new_offset = new_offset;
        lr->deleted_id = deleted_id;
        lr->operation = operation;
    }
};

}  // namespace db
}  // namespace gaia
