/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "db_helpers.hpp"
#include "db_server.hpp"
#include "db_shared_data.hpp"

gaia::db::locators_t* gaia::db::get_locators()
{
    // The local snapshot segment should always be mapped whenever any callers
    // of this method are able to observe it.
    ASSERT_PRECONDITION(gaia::db::server_t::s_local_snapshot_locators.is_set(), "Invalid local snapshot!");
    return gaia::db::server_t::s_local_snapshot_locators.data();
}

gaia::db::locators_t* gaia::db::get_locators_for_allocator()
{
    // The shared locator segment should always be mapped whenever any callers
    // of this method are able to observe it.
    ASSERT_PRECONDITION(gaia::db::server_t::s_shared_locators.is_set(), "Invalid local snapshot!");
    return gaia::db::server_t::s_shared_locators.data();
}

gaia::db::counters_t* gaia::db::get_counters()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the counters segment unmapped).
    ASSERT_PRECONDITION(gaia::db::server_t::s_shared_counters.is_set(), "Server counters segment is unmapped!");
    return gaia::db::server_t::s_shared_counters.data();
}

gaia::db::data_t* gaia::db::get_data()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the data segment unmapped).
    ASSERT_PRECONDITION(gaia::db::server_t::s_shared_data.is_set(), "Server data segment is unmapped!");
    return gaia::db::server_t::s_shared_data.data();
}

gaia::db::logs_t* gaia::db::get_logs()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the data segment unmapped).
    ASSERT_PRECONDITION(gaia::db::server_t::s_shared_logs.is_set(), "Server logs segment is unmapped!");
    return gaia::db::server_t::s_shared_logs.data();
}

gaia::db::id_index_t* gaia::db::get_id_index()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the id_index segment unmapped).
    ASSERT_PRECONDITION(gaia::db::server_t::s_shared_id_index.is_set(), "Server id_index segment is unmapped!");
    return gaia::db::server_t::s_shared_id_index.data();
}

gaia::db::type_index_t* gaia::db::get_type_index()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the type_index segment unmapped).
    ASSERT_PRECONDITION(gaia::db::server_t::s_shared_type_index.is_set(), "Server type_index segment is unmapped!");
    return gaia::db::server_t::s_shared_type_index.data();
}

gaia::db::gaia_txn_id_t gaia::db::get_current_txn_id()
{
    return gaia::db::server_t::s_txn_id;
}

gaia::db::index::indexes_t* gaia::db::get_indexes()
{
    return &gaia::db::server_t::s_global_indexes;
}

gaia::db::memory_manager::memory_manager_t* gaia::db::get_memory_manager()
{
    return &gaia::db::server_t::s_memory_manager;
}

gaia::db::memory_manager::chunk_manager_t* gaia::db::get_chunk_manager()
{
    return &gaia::db::server_t::s_chunk_manager;
}

gaia::db::txn_log_t* gaia::db::get_txn_log()
{
    return gaia::db::get_txn_log_from_offset(gaia::db::server_t::s_txn_log_offset);
}
