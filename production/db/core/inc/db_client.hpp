/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>
#include <optional>

#include <sys/socket.h>

#include "gaia/db/db.hpp"

#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/common/mmap_helpers.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/system_table_types.hpp"
#include "gaia_internal/db/db_client_config.hpp"
#include "gaia_internal/db/triggers.hpp"

#include "client_messenger.hpp"
#include "db_shared_data.hpp"
#include "mapped_data.hpp"
#include "memory_manager.hpp"
#include "messages_generated.h"

namespace gaia
{
namespace db
{

namespace query_processor
{
class db_client_proxy_t;
} // namespace query_processor

class client_t
{
    friend class gaia_ptr_t;

    /**
     * @throws no_open_transaction if there is no open transaction.
     */
    friend gaia::db::locators_t* gaia::db::get_locators();

    /**
     * @throws no_open_session if there is no open session.
     */
    friend gaia::db::counters_t* gaia::db::get_counters();
    friend gaia::db::data_t* gaia::db::get_data();
    friend gaia::db::id_index_t* gaia::db::get_id_index();
    friend gaia::db::index::indexes_t* gaia::db::get_indexes();

    friend gaia::db::memory_manager::address_offset_t gaia::db::allocate_object(
        gaia_locator_t locator,
        size_t size);

    friend class gaia::db::query_processor::db_client_proxy_t;

public:
    static inline bool is_session_open();
    static inline bool is_transaction_open();

    /**
     * Called by the rules engine only during initialization and
     * shutdown.
     */
    static inline void set_commit_trigger(triggers::commit_trigger_fn trigger_fn);

    // This test-only function is exported from gaia_db_internal.hpp.
    static void clear_shared_memory();

    // These public functions are exported from and documented in db.hpp.
    static void begin_session(config::session_options_t session_options);
    static void end_session();
    static void begin_transaction();
    static void rollback_transaction();
    static void commit_transaction();

    static inline gaia_txn_id_t get_txn_id();
    static inline int get_session_socket_for_txn();

    // This returns a generator object for gaia_ids of a given type.
    static std::shared_ptr<common::iterators::generator_t<common::gaia_id_t>>
    get_id_generator_for_type(common::gaia_type_t type, gaia_txn_id_t txn_id);

    // This is a helper for higher-level methods that use
    // this generator to build a range or iterator object.
    template <typename T_element_type>
    static std::function<std::optional<T_element_type>()>
    get_stream_generator_for_socket(std::shared_ptr<int> stream_socket_ptr);

private:
    // These fields have transaction lifetime.
    thread_local static inline gaia_txn_id_t s_txn_id = c_invalid_gaia_txn_id;
    thread_local static inline mapped_log_t s_log{};

    thread_local static inline mapped_data_t<locators_t> s_private_locators;
    thread_local static inline gaia::db::index::indexes_t s_local_indexes{};

    // These fields have session lifetime.
    thread_local static inline config::session_options_t s_session_options;

    thread_local static inline int s_fd_locators = -1;

    thread_local static inline mapped_data_t<counters_t> s_shared_counters;
    thread_local static inline mapped_data_t<data_t> s_shared_data;
    thread_local static inline mapped_data_t<id_index_t> s_shared_id_index;

    thread_local static inline int s_session_socket = -1;

    // A list of data mappings that we manage together.
    // The order of declarations must be the order of data_mapping_t::index_t values!
    thread_local static inline data_mapping_t s_data_mappings[] = {
        {data_mapping_t::index_t::locators, &s_private_locators, c_gaia_mem_locators_prefix},
        {data_mapping_t::index_t::counters, &s_shared_counters, c_gaia_mem_counters_prefix},
        {data_mapping_t::index_t::data, &s_shared_data, c_gaia_mem_data_prefix},
        {data_mapping_t::index_t::id_index, &s_shared_id_index, c_gaia_mem_id_index_prefix},
    };

    // s_events has transaction lifetime and is cleared after each transaction.
    // Set by the rules engine.
    thread_local static inline std::vector<gaia::db::triggers::trigger_event_t> s_events{};
    static inline triggers::commit_trigger_fn s_txn_commit_trigger = nullptr;

    thread_local static inline gaia::db::memory_manager::memory_manager_t s_memory_manager{};
    thread_local static inline gaia::db::memory_manager::chunk_manager_t s_chunk_manager{};
    thread_local static inline std::vector<gaia::db::memory_manager::chunk_manager_t> s_previous_chunk_managers{};

private:
    static void init_memory_manager();

    static gaia::db::memory_manager::address_offset_t allocate_object(
        gaia_locator_t locator,
        size_t size);

    static void txn_cleanup();

    static void commit_chunk_manager_allocations();
    static void rollback_chunk_manager_allocations();

    static void apply_txn_log(int log_fd);

    static int get_session_socket(const std::string& socket_name);

    static std::shared_ptr<int> get_id_cursor_socket_for_type(common::gaia_type_t type);

    static std::function<std::optional<int>()>
    get_fd_stream_generator_for_socket(int stream_socket);

    static std::function<std::optional<common::gaia_id_t>()>
    augment_id_generator_for_type(common::gaia_type_t type, std::function<std::optional<common::gaia_id_t>()> id_generator);

    /**
     *  Check if an event should be generated for a given type.
     */
    static inline bool is_valid_event(common::gaia_type_t type);

    static inline void verify_txn_active();
    static inline void verify_no_txn();

    static inline void verify_session_active();
    static inline void verify_no_session();

    static inline void txn_log(
        gaia_locator_t locator,
        gaia_offset_t old_offset,
        gaia_offset_t new_offset,
        gaia_operation_t operation,
        // `deleted_id` is required to keep track of deleted keys which will be propagated to the persistent layer.
        // Memory for other operations will be unused. An alternative would be to keep a separate log for deleted keys only.
        common::gaia_id_t deleted_id = common::c_invalid_gaia_id);
};

#include "db_client.inc"

} // namespace db
} // namespace gaia
