/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "db_client.hpp"
#include "db_helpers.hpp"
#include "db_shared_data.hpp"

const bool gaia::db::c_is_running_on_server = false;
const bool gaia::db::c_is_running_on_client = true;

gaia::db::locators_t* gaia::db::get_locators()
{
    DEBUG_ASSERT_PRECONDITION(gaia::db::client_t::s_private_locators.is_set(), "Locators segments is unmapped!");

    // REVIEW: Callers of this method should probably never be able to observe
    // the locators segment in an unmapped state (i.e., outside a transaction),
    // but asserting this condition currently breaks test code that assumes we
    // should instead throw a `transaction_not_open` exception. We need to
    // determine whether throwing an exception is even appropriate (since
    // observing a null locators pointer should always be the result of a
    // programming error, i.e., calling a transactional operation outside a
    // transaction) before adding an assert here.
    return gaia::db::client_t::s_private_locators.data();
}

gaia::db::locators_t* gaia::db::get_locators_for_allocator()
{
    return gaia::db::get_locators();
}

// Since we don't use this accessor in the client itself, we can assert that
// it is always non-null (since callers should never be able to observe it
// in its null state, i.e., with the counters segment unmapped).
gaia::db::counters_t* gaia::db::get_counters()
{
    DEBUG_ASSERT_PRECONDITION(gaia::db::client_t::s_shared_counters.is_set(), "Counters segment is unmapped!");

    return gaia::db::client_t::s_shared_counters.data();
}

// Since we don't use this accessor in the client itself, we can assert that
// it is always non-null (since callers should never be able to observe it
// in its null state, i.e., with the data segment unmapped).
gaia::db::data_t* gaia::db::get_data()
{
    DEBUG_ASSERT_PRECONDITION(gaia::db::client_t::s_shared_data.is_set(), "Data segment is unmapped!");

    return gaia::db::client_t::s_shared_data.data();
}

// Since we don't use this accessor in the client itself, we can assert that
// it is always non-null (since callers should never be able to observe it
// in its null state, i.e., with the data segment unmapped).
gaia::db::logs_t* gaia::db::get_logs()
{
    DEBUG_ASSERT_PRECONDITION(gaia::db::client_t::s_shared_logs.is_set(), "Log segment is unmapped!");

    return gaia::db::client_t::s_shared_logs.data();
}

// Since we don't use this accessor in the client itself, we can assert that
// it is always non-null (since callers should never be able to observe it
// in its null state, i.e., with the id_index segment unmapped).
gaia::db::id_index_t* gaia::db::get_id_index()
{
    DEBUG_ASSERT_PRECONDITION(gaia::db::client_t::s_shared_id_index.is_set(), "Id index segment is unmapped!");

    return gaia::db::client_t::s_shared_id_index.data();
}

// Since we don't use this accessor in the client itself, we can assert that
// it is always non-null (since callers should never be able to observe it
// in its null state, i.e., with the type_index segment unmapped).
gaia::db::type_index_t* gaia::db::get_type_index()
{
    DEBUG_ASSERT_PRECONDITION(gaia::db::client_t::s_shared_type_index.is_set(), "Type index segment is unmapped!");

    return gaia::db::client_t::s_shared_type_index.data();
}

gaia::db::gaia_txn_id_t gaia::db::get_current_txn_id()
{
    return gaia::db::client_t::s_txn_id;
}

gaia::db::index::indexes_t* gaia::db::get_indexes()
{
    return &gaia::db::client_t::s_local_indexes;
}

gaia::db::memory_manager::memory_manager_t* gaia::db::get_memory_manager()
{
    return &gaia::db::client_t::s_memory_manager;
}

gaia::db::memory_manager::chunk_manager_t* gaia::db::get_chunk_manager()
{
    return &gaia::db::client_t::s_chunk_manager;
}

gaia::db::txn_log_t* gaia::db::get_txn_log()
{
    DEBUG_ASSERT_PRECONDITION(gaia::db::client_t::s_txn_log_offset.is_valid(), "Transaction log is uninitialized!");

    return gaia::db::get_txn_log_from_offset(gaia::db::client_t::s_txn_log_offset);
}
