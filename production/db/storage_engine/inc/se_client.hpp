/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/db/db.hpp"
#include "memory_types.hpp"
#include "messages_generated.h"
#include "retail_assert.hpp"
#include "se_shared_data.hpp"
#include "stack_allocator.hpp"
#include "system_table_types.hpp"
#include "triggers.hpp"

namespace gaia
{

namespace db
{

class client
{
    friend class gaia_ptr;

    /**
     * @throws no_open_transaction if there is no active transaction.
     */
    friend gaia::db::locators_t* gaia::db::get_shared_locators();

    /**
     * @throws no_active_session if there is no active session.
     */
    friend gaia::db::shared_counters_t* gaia::db::get_shared_counters();
    friend gaia::db::shared_data_t* gaia::db::get_shared_data();
    friend gaia::db::shared_id_index_t* gaia::db::get_shared_id_index();

    friend gaia::db::memory_manager::address_offset_t gaia::db::allocate_object(
        gaia_locator_t locator,
        gaia::db::memory_manager::address_offset_t old_slot_offset,
        size_t size);

public:
    static inline bool is_transaction_active()
    {
        return (s_locators != nullptr);
    }

    /**
     * Called by the rules engine only during initialization and
     * shutdown.
     */
    static inline void set_commit_trigger(triggers::commit_trigger_fn trigger_fn)
    {
        s_txn_commit_trigger = trigger_fn;
    }

    // This test-only function is exported from gaia_db_internal.hpp.
    static void clear_shared_memory();

    // These public functions are exported from and documented in db.hpp.
    static void begin_session();
    static void end_session();
    static void begin_transaction();
    static void rollback_transaction();
    static void commit_transaction();

    // This returns a generator object for gaia_ids of a given type.
    static std::function<std::optional<gaia::common::gaia_id_t>()> get_id_generator_for_type(gaia_type_t type);

    // Make IPC call to the server requesting more memory for the current transaction
    // in case the client runs out of memory mid transaction.
    static void request_memory();

private:
    // These fields have transaction lifetime.
    thread_local static inline txn_log_t* s_log = nullptr;
    thread_local static inline int s_fd_log = -1;
    thread_local static inline locators_t* s_locators = nullptr;

    // These fields have session lifetime.
    thread_local static inline int s_fd_locators = -1;
    thread_local static inline shared_counters_t* s_counters = nullptr;
    thread_local static inline shared_data_t* s_data = nullptr;
    thread_local static inline shared_id_index_t* s_id_index = nullptr;
    thread_local static inline int s_session_socket = -1;
    thread_local static inline gaia_txn_id_t s_txn_id = c_invalid_gaia_txn_id;
    thread_local static inline std::unique_ptr<gaia::db::memory_manager::stack_allocator_t> s_current_stack_allocator{};

    // s_events has transaction lifetime and is cleared after each transaction.
    // Set by the rules engine.
    thread_local static inline std::vector<gaia::db::triggers::trigger_event_t> s_events{};
    static inline triggers::commit_trigger_fn s_txn_commit_trigger = nullptr;

    // Maintain a static filter in the client to disable generating events
    // for system types.
    static constexpr gaia_type_t c_trigger_excluded_types[] = {
        static_cast<gaia_type_t>(system_table_type_t::catalog_gaia_table),
        static_cast<gaia_type_t>(system_table_type_t::catalog_gaia_field),
        static_cast<gaia_type_t>(system_table_type_t::catalog_gaia_relationship),
        static_cast<gaia_type_t>(system_table_type_t::catalog_gaia_ruleset),
        static_cast<gaia_type_t>(system_table_type_t::catalog_gaia_rule),
        static_cast<gaia_type_t>(system_table_type_t::event_log)};

    // The largest object size of 64KB won't fit into a stack allocator of size 64KB
    // due to other metadata created by the stack allocator, which is why we allot an additional 128 bytes of memory
    // to the initial stack allocator size per transaction.
    static constexpr size_t c_initial_txn_memory_size_bytes = 64 * 1024 + 128;

    // Note that the transaction will incrementally request more memory for the stack allocator upto a certain maximum size
    // if it keeps running out of memory.
    static constexpr size_t c_max_memory_request_size_bytes = 16 * c_initial_txn_memory_size_bytes;
    static constexpr int c_memory_request_size_multiplier = 2;

    // Maintain a thread local variable to track the requested memory allocation size for the current transaction.
    thread_local static inline size_t s_txn_memory_request_size = c_initial_txn_memory_size_bytes;

    // Load server initialized stack allocator on the client.
    static void load_stack_allocator(const messages::memory_allocation_info_t* allocation_info, uint8_t* data_mapping_base_addr);

    static gaia::db::memory_manager::address_offset_t allocate_object(
        gaia_locator_t locator,
        gaia::db::memory_manager::address_offset_t old_slot_offset,
        size_t size);

    static gaia::db::memory_manager::address_offset_t stack_allocator_allocate(
        gaia_locator_t locator,
        gaia::db::memory_manager::address_offset_t old_slot_offset,
        size_t size);

    static void txn_cleanup();

    static void sort_log();

    static void dedup_log();

    static void apply_txn_log(int log_fd);

    static int get_session_socket();

    static int get_id_cursor_socket_for_type(gaia_type_t type);

    // This is a helper for higher-level methods that use
    // this generator to build a range or iterator object.
    template <typename T_element_type>
    static std::function<std::optional<T_element_type>()>
    get_stream_generator_for_socket(int stream_socket);

    static std::function<std::optional<int>()>
    get_fd_stream_generator_for_socket(int stream_socket);

    /**
     *  Check if an event should be generated for a given type.
     */
    static inline bool is_valid_event(gaia_type_t type)
    {
        constexpr const gaia_type_t* c_end = c_trigger_excluded_types + std::size(c_trigger_excluded_types);
        return (s_txn_commit_trigger && (std::find(c_trigger_excluded_types, c_end, type) == c_end));
    }

    static inline void verify_txn_active()
    {
        if (!is_transaction_active())
        {
            throw no_open_transaction();
        }
    }

    static inline void verify_no_txn()
    {
        if (is_transaction_active())
        {
            throw transaction_in_progress();
        }
    }

    static inline void verify_session_active()
    {
        if (s_session_socket == -1)
        {
            throw no_active_session();
        }
    }

    static inline void verify_no_session()
    {
        if (s_session_socket != -1)
        {
            throw session_exists();
        }
    }

    static inline void txn_log(
        gaia_locator_t locator,
        gaia_offset_t old_offset,
        gaia_offset_t new_offset,
        gaia_operation_t operation,
        // `deleted_id` is required to keep track of deleted keys which will be propagated to the persistent layer.
        // Memory for other operations will be unused. An alternative would be to keep a separate log for deleted keys only.
        gaia::common::gaia_id_t deleted_id = c_invalid_gaia_id)
    {
        if (operation == gaia_operation_t::remove)
        {
            gaia::common::retail_assert(
                deleted_id != c_invalid_gaia_id && new_offset == c_invalid_gaia_offset,
                "A delete operation must have a valid deleted gaia_id and an invalid new version offset!");
        }

        // We never allocate more than `c_max_log_records` records in the log.
        if (s_log->count == c_max_log_records)
        {
            throw transaction_object_limit_exceeded();
        }

        // Initialize the new record and increment the record count.
        txn_log_t::log_record_t* lr = s_log->log_records + s_log->count++;
        lr->locator = locator;
        lr->old_offset = old_offset;
        lr->new_offset = new_offset;
        lr->deleted_id = deleted_id;
        lr->operation = operation;
    }
};

} // namespace db
} // namespace gaia
