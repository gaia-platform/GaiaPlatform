/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

static const bool c_is_running_on_server = true;
static const bool c_is_running_on_client = false;

static gaia::db::locators_t* get_locators()
{
    // The local snapshot segment should always be mapped whenever any callers
    // of this method are able to observe it.
    DEBUG_ASSERT_PRECONDITION(gaia::db::server_t::s_local_snapshot_locators.is_set(), "Invalid local snapshot!");
    return gaia::db::server_t::s_local_snapshot_locators.data();
}

static gaia::db::locators_t* get_locators_for_allocator()
{
    // The shared locator segment should always be mapped whenever any callers
    // of this method are able to observe it.
    DEBUG_ASSERT_PRECONDITION(gaia::db::server_t::s_shared_locators.is_set(), "Invalid local snapshot!");
    return gaia::db::server_t::s_shared_locators.data();
}

static gaia::db::counters_t* get_counters()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the counters segment unmapped).
    DEBUG_ASSERT_PRECONDITION(gaia::db::server_t::s_shared_counters.is_set(), "Server counters segment is unmapped!");
    return gaia::db::server_t::s_shared_counters.data();
}

static gaia::db::data_t* get_data()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the data segment unmapped).
    DEBUG_ASSERT_PRECONDITION(gaia::db::server_t::s_shared_data.is_set(), "Server data segment is unmapped!");
    return gaia::db::server_t::s_shared_data.data();
}

static gaia::db::logs_t* get_logs()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the data segment unmapped).
    DEBUG_ASSERT_PRECONDITION(gaia::db::server_t::s_shared_logs.is_set(), "Server logs segment is unmapped!");
    return gaia::db::server_t::s_shared_logs.data();
}

static gaia::db::id_index_t* get_id_index()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the id_index segment unmapped).
    DEBUG_ASSERT_PRECONDITION(gaia::db::server_t::s_shared_id_index.is_set(), "Server id_index segment is unmapped!");
    return gaia::db::server_t::s_shared_id_index.data();
}

static gaia::db::type_index_t* get_type_index()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the type_index segment unmapped).
    DEBUG_ASSERT_PRECONDITION(gaia::db::server_t::s_shared_type_index.is_set(), "Server type_index segment is unmapped!");
    return gaia::db::server_t::s_shared_type_index.data();
}

static gaia::db::index::indexes_t* get_indexes()
{
    return &gaia::db::server_t::s_global_indexes;
}

static gaia::db::memory_manager::memory_manager_t* get_memory_manager()
{
    return &gaia::db::server_t::s_memory_manager;
}

static gaia::db::memory_manager::chunk_manager_t* get_chunk_manager()
{
    return &gaia::db::server_t::s_chunk_manager;
}

static gaia::db::txn_log_t* get_txn_log_from_offset(gaia::db::log_offset_t offset)
{
    DEBUG_ASSERT_PRECONDITION(offset.is_valid(), "Txn log offset is invalid!");

    gaia::db::logs_t* logs = get_logs();
    return &((*logs)[offset]);
}

static gaia::db::txn_log_t* get_txn_log()
{
    return get_txn_log_from_offset(gaia::db::server_t::s_txn_log_offset);
}

static gaia::db::gaia_txn_id_t get_txn_id()
{
    return gaia::db::server_t::s_txn_id;
}

static const gaia::db::caches::db_caches_t* get_db_caches()
{
    return nullptr;
}
