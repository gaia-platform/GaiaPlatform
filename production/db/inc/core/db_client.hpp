/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>
#include <mutex>
#include <optional>

#include <sys/socket.h>

#include "gaia/db/db.hpp"

#include "gaia_internal/common/assert.hpp"
#include "gaia_internal/common/debug_assert.hpp"
#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/common/mmap_helpers.hpp"
#include "gaia_internal/common/system_table_types.hpp"
#include "gaia_internal/db/db_client_config.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"
#include "gaia_internal/db/triggers.hpp"
#include "gaia_internal/exceptions.hpp"

#include "chunk_manager.hpp"
#include "client_messenger.hpp"
#include "db_caches.hpp"
#include "mapped_data.hpp"
#include "memory_manager.hpp"
#include "messages_generated.h"
#include "type_index.hpp"

namespace gaia
{
namespace db
{

namespace query_processor
{
class db_client_proxy_t;
} // namespace query_processor

// For declarations of friend functions.
#include "db_shared_data_interface.inc"

class client_t
{
    /**
     * @throws no_open_transaction_internal if there is no open transaction.
     */
    friend gaia::db::locators_t* gaia::db::get_locators();
    friend gaia::db::txn_log_t* gaia::db::get_txn_log();

    /**
     * @throws no_open_session_internal if there is no open session.
     */
    friend gaia::db::counters_t* gaia::db::get_counters();
    friend gaia::db::data_t* gaia::db::get_data();
    friend gaia::db::logs_t* gaia::db::get_logs();
    friend gaia::db::id_index_t* gaia::db::get_id_index();
    friend gaia::db::type_index_t* gaia::db::get_type_index();
    friend gaia::db::index::indexes_t* gaia::db::get_indexes();
    friend gaia::db::memory_manager::memory_manager_t* gaia::db::get_memory_manager();
    friend gaia::db::memory_manager::chunk_manager_t* gaia::db::get_chunk_manager();
    friend gaia::db::gaia_txn_id_t gaia::db::get_txn_id();
    friend const gaia::db::caches::db_caches_t* gaia::db::get_db_caches();

    friend class gaia::db::query_processor::db_client_proxy_t;

public:
    // These functions are exported from gaia_internal/db/db.hpp.
    static inline gaia_txn_id_t get_current_txn_id();
    static void clear_shared_memory();
    static inline void set_commit_trigger(triggers::commit_trigger_fn trigger_fn);
    static inline bool has_commit_trigger();
    static inline bool is_ping_session_open();
    static inline bool is_ddl_session_open();

    // These functions are exported from and documented in gaia/db/db.hpp.
    static inline bool is_session_open();
    static inline bool is_transaction_open();
    static void end_session();
    static void begin_transaction();
    static void rollback_transaction();
    static void commit_transaction();

    // Internal version of begin_session(), called by public interface in db.hpp.
    static void begin_session(config::session_options_t session_options);

    static inline int get_session_socket_for_txn();

    // This returns a generator object for locators of a given type.
    static std::shared_ptr<common::iterators::generator_t<gaia_locator_t>>
    get_locator_generator_for_type(common::gaia_type_t type);

    // This is a helper for higher-level methods that use
    // this generator to build a range or iterator object.
    template <typename T_element_type>
    static std::function<std::optional<T_element_type>()>
    get_stream_generator_for_socket(std::shared_ptr<int> stream_socket_ptr);

    // Called by internal code to verify preconditions.
    static inline void verify_txn_active();
    static inline void verify_no_txn();

    static inline void verify_session_active();
    static inline void verify_no_session();

    // Called by internal code to log events for rule invocations.
    static inline bool is_valid_event(common::gaia_type_t type);
    static inline void log_event(
        triggers::event_type_t event_type, common::gaia_type_t type,
        common::gaia_id_t id, common::field_position_list_t changed_fields);

private:
    // These fields have transaction lifetime.
    thread_local static inline gaia_txn_id_t s_txn_id = c_invalid_gaia_txn_id;
    thread_local static inline log_offset_t s_txn_log_offset = c_invalid_log_offset;

    // NB: We need to use the (nonstandard) __thread attribute rather than
    // thread_local to ensure that a minimal TLS implementation is used.
    __thread static inline mapped_data_t<locators_t> s_private_locators{};

    thread_local static inline gaia::db::index::indexes_t s_local_indexes{};

    // These fields have session lifetime.
    thread_local static inline config::session_options_t s_session_options;

    // We don't use an auto-pointer because its destructor is "non-trivial"
    // and that would add overhead to the TLS implementation.
    thread_local static inline gaia::db::caches::db_caches_t* s_db_caches_ptr{nullptr};

    // REVIEW: When we enable snapshot reuse across txns (by applying the undo
    // log from the previous txn to the existing snapshot and then applying redo
    // logs from txns committed after the last shared locators view update), we
    // need to track the last commit_ts whose log was applied to the snapshot,
    // so we can ignore any logs committed at or before that commit_ts.
    thread_local static inline gaia_txn_id_t s_latest_applied_commit_ts = c_invalid_gaia_txn_id;

    thread_local static inline int s_fd_locators = -1;

    // NB: We need to use the (nonstandard) __thread attribute rather than
    // thread_local to ensure that a minimal TLS implementation is used.
    __thread static inline mapped_data_t<counters_t> s_shared_counters{};
    __thread static inline mapped_data_t<data_t> s_shared_data{};
    __thread static inline mapped_data_t<logs_t> s_shared_logs{};
    __thread static inline mapped_data_t<id_index_t> s_shared_id_index{};
    __thread static inline mapped_data_t<type_index_t> s_shared_type_index{};

    thread_local static inline gaia::db::memory_manager::memory_manager_t s_memory_manager{};
    thread_local static inline gaia::db::memory_manager::chunk_manager_t s_chunk_manager{};

    thread_local static inline int s_session_socket = -1;

    // A log processing watermark that is used for index maintenance.
    thread_local static inline size_t s_last_index_processed_log_count = 0;

    // A list of data mappings that we manage together.
    // The order of declarations must be the order of data_mapping_t::index_t values!
    thread_local static inline data_mapping_t s_data_mappings[] = {
        {data_mapping_t::index_t::locators, &s_private_locators, c_gaia_mem_locators_prefix},
        {data_mapping_t::index_t::counters, &s_shared_counters, c_gaia_mem_counters_prefix},
        {data_mapping_t::index_t::data, &s_shared_data, c_gaia_mem_data_prefix},
        {data_mapping_t::index_t::logs, &s_shared_logs, c_gaia_mem_logs_prefix},
        {data_mapping_t::index_t::id_index, &s_shared_id_index, c_gaia_mem_id_index_prefix},
        {data_mapping_t::index_t::type_index, &s_shared_type_index, c_gaia_mem_type_index_prefix},
    };

    // s_events has transaction lifetime and is cleared after each transaction.
    // Set by the rules engine.
    thread_local static inline std::vector<gaia::db::triggers::trigger_event_t> s_events{};
    static inline triggers::commit_trigger_fn s_txn_commit_trigger = nullptr;

private:
    static void init_memory_manager();

    static gaia::db::caches::db_caches_t* init_db_caches();

    static void txn_cleanup();

    static void commit_chunk_manager_allocations();
    static void rollback_chunk_manager_allocations();

    static void apply_txn_log(log_offset_t offset);

    static int get_session_socket(const std::string& socket_name);
};

#include "db_client.inc"

} // namespace db
} // namespace gaia
