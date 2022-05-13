/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

// Returns a pointer to a mapping of the "locators" shared memory segment.
static inline gaia::db::locators_t* get_locators();

// Returns a pointer to a mapping of the "locators" shared memory segment for the allocator.
static inline gaia::db::locators_t* get_locators_for_allocator();

// Returns a pointer to a mapping of the "counters" shared memory segment.
static inline gaia::db::counters_t* get_counters();

// Returns a pointer to a mapping of the "data" shared memory segment.
static inline gaia::db::data_t* get_data();

// Returns a pointer to a mapping of the "logs" shared memory segment.
static inline gaia::db::logs_t* get_logs();

// Returns a pointer to a mapping of the "id_index" shared memory segment.
static inline gaia::db::id_index_t* get_id_index();

// Returns a pointer to a mapping of the "type_index" shared memory segment.
static inline gaia::db::type_index_t* get_type_index();

// Returns a pointer to the indexes.
static inline gaia::db::index::indexes_t* get_indexes();

// Gets the memory manager instance for the current thread or process.
static inline gaia::db::memory_manager::memory_manager_t* get_memory_manager();

// Gets the chunk manager instance for the current thread or process.
static inline gaia::db::memory_manager::chunk_manager_t* get_chunk_manager();

// Gets the transaction log at the given offset.
static inline gaia::db::txn_log_t* get_txn_log_from_offset(gaia::db::log_offset_t offset);

// Gets the transaction log for the current session thread.
static inline gaia::db::txn_log_t* get_txn_log();

// Gets the transaction ID for the current session thread.
static inline gaia::db::gaia_txn_id_t get_txn_id();

// Returns a reference to the db_caches structure.
static inline const gaia::db::caches::db_caches_t* get_db_caches();
