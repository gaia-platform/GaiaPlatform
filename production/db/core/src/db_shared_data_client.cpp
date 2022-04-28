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
    // Since we don't use this accessor in the client itself, we can assert that
    // it is always non-null (since callers should never be able to observe it
    // in its null state, i.e., with the locators segment unmapped).
    DEBUG_ASSERT_PRECONDITION(gaia::db::client_t::s_private_locators.is_set(), "Locators segment not mapped!");

    return gaia::db::client_t::s_private_locators.data();
}

gaia::db::locators_t* gaia::db::get_locators_for_allocator()
{
    return gaia::db::get_locators();
}

gaia::db::counters_t* gaia::db::get_counters()
{
    // Since we don't use this accessor in the client itself, we can assert that
    // it is always non-null (since callers should never be able to observe it
    // in its null state, i.e., with the counters segment unmapped).
    DEBUG_ASSERT_PRECONDITION(gaia::db::client_t::s_shared_counters.is_set(), "Shared counters not mapped!");

    return gaia::db::client_t::s_shared_counters.data();
}

gaia::db::data_t* gaia::db::get_data()
{
    // Since we don't use this accessor in the client itself, we can assert that
    // it is always non-null (since callers should never be able to observe it
    // in its null state, i.e., with the data segment unmapped).
    DEBUG_ASSERT_PRECONDITION(gaia::db::client_t::s_shared_data.is_set(), "Data segment not mapped!");

    return gaia::db::client_t::s_shared_data.data();
}

gaia::db::logs_t* gaia::db::get_logs()
{
    // Since we don't use this accessor in the client itself, we can assert that
    // it is always non-null (since callers should never be able to observe it
    // in its null state, i.e., with the data segment unmapped).
    DEBUG_ASSERT_PRECONDITION(gaia::db::client_t::s_shared_logs.is_set(), "Txn log segment not mapped!");

    return gaia::db::client_t::s_shared_logs.data();
}

gaia::db::id_index_t* gaia::db::get_id_index()
{
    // Since we don't use this accessor in the client itself, we can assert that
    // it is always non-null (since callers should never be able to observe it
    // in its null state, i.e., with the id_index segment unmapped).
    DEBUG_ASSERT_PRECONDITION(gaia::db::client_t::s_shared_id_index.is_set(), "ID index not mapped!");

    return gaia::db::client_t::s_shared_id_index.data();
}

gaia::db::type_index_t* gaia::db::get_type_index()
{
    // Since we don't use this accessor in the client itself, we can assert that
    // it is always non-null (since callers should never be able to observe it
    // in its null state, i.e., with the type_index segment unmapped).
    DEBUG_ASSERT_PRECONDITION(gaia::db::client_t::s_shared_type_index.is_set(), "Type index not mapped!");

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

gaia::db::memory_manager_t* gaia::db::get_memory_manager()
{
    return &gaia::db::client_t::s_memory_manager;
}

gaia::db::chunk_manager_t* gaia::db::get_chunk_manager()
{
    return &gaia::db::client_t::s_chunk_manager;
}

gaia::db::txn_log_t* gaia::db::get_txn_log()
{
    // Since we don't use this accessor in the client itself, we can assert that
    // it is always non-null (since callers should never be able to observe it
    // in its null state, i.e., with the id_index segment unmapped).
    DEBUG_ASSERT_PRECONDITION(gaia::db::client_t::s_txn_log_offset.is_valid(), "Invalid txn log offset!");

    return gaia::db::get_txn_log_from_offset(gaia::db::client_t::s_txn_log_offset);
}
