////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

inline const bool c_is_running_on_server = false;
inline const bool c_is_running_on_client = true;

static gaia::db::locators_t* get_locators()
{
    // Since we don't use this accessor in the client itself, we can assert that
    // it is always non-null (since callers should never be able to observe it
    // in its null state, i.e., with the locators segment unmapped).
    DEBUG_ASSERT_PRECONDITION(
        gaia::db::client_t::s_session_context && gaia::db::client_t::s_session_context->private_locators.is_set(),
        "Locators segment not mapped!");

    return gaia::db::client_t::s_session_context->private_locators.data();
}

static gaia::db::locators_t* get_locators_for_allocator()
{
    return get_locators();
}

static gaia::db::counters_t* get_counters()
{
    // Since we don't use this accessor in the client itself, we can assert that
    // it is always non-null (since callers should never be able to observe it
    // in its null state, i.e., with the counters segment unmapped).
    DEBUG_ASSERT_PRECONDITION(
        gaia::db::client_t::s_session_context && gaia::db::client_t::s_session_context->shared_counters.is_set(),
        "Shared counters not mapped!");

    return gaia::db::client_t::s_session_context->shared_counters.data();
}

static gaia::db::data_t* get_data()
{
    // Since we don't use this accessor in the client itself, we can assert that
    // it is always non-null (since callers should never be able to observe it
    // in its null state, i.e., with the data segment unmapped).
    DEBUG_ASSERT_PRECONDITION(
        gaia::db::client_t::s_session_context && gaia::db::client_t::s_session_context->shared_data.is_set(),
        "Data segment not mapped!");

    return gaia::db::client_t::s_session_context->shared_data.data();
}

static gaia::db::logs_t* get_logs()
{
    // Since we don't use this accessor in the client itself, we can assert that
    // it is always non-null (since callers should never be able to observe it
    // in its null state, i.e., with the data segment unmapped).
    DEBUG_ASSERT_PRECONDITION(
        gaia::db::client_t::s_session_context && gaia::db::client_t::s_session_context->shared_logs.is_set(),
        "Txn log segment not mapped!");

    return gaia::db::client_t::s_session_context->shared_logs.data();
}

static gaia::db::id_index_t* get_id_index()
{
    // Since we don't use this accessor in the client itself, we can assert that
    // it is always non-null (since callers should never be able to observe it
    // in its null state, i.e., with the id_index segment unmapped).
    DEBUG_ASSERT_PRECONDITION(
        gaia::db::client_t::s_session_context && gaia::db::client_t::s_session_context->shared_id_index.is_set(),
        "ID index not mapped!");

    return gaia::db::client_t::s_session_context->shared_id_index.data();
}

static gaia::db::type_index_t* get_type_index()
{
    // Since we don't use this accessor in the client itself, we can assert that
    // it is always non-null (since callers should never be able to observe it
    // in its null state, i.e., with the type_index segment unmapped).
    DEBUG_ASSERT_PRECONDITION(
        gaia::db::client_t::s_session_context && gaia::db::client_t::s_session_context->shared_type_index.is_set(),
        "Type index not mapped!");

    return gaia::db::client_t::s_session_context->shared_type_index.data();
}

static gaia::db::index::indexes_t* get_indexes()
{
    return &gaia::db::client_t::s_session_context->txn_context->local_indexes;
}

static gaia::db::memory_manager::memory_manager_t* get_memory_manager()
{
    return &gaia::db::client_t::s_session_context->memory_manager;
}

static gaia::db::memory_manager::chunk_manager_t* get_chunk_manager()
{
    return &gaia::db::client_t::s_session_context->chunk_manager;
}

static gaia::db::txn_log_t* get_txn_log_from_offset(gaia::db::log_offset_t offset)
{
    DEBUG_ASSERT_PRECONDITION(offset.is_valid(), "Txn log offset is invalid!");

    gaia::db::logs_t* logs = get_logs();
    return &((*logs)[offset]);
}

static gaia::db::txn_log_t* get_txn_log()
{
    // Since we don't use this accessor in the client itself, we can assert that
    // it is always non-null (since callers should never be able to observe it
    // in its null state, i.e., with the id_index segment unmapped).
    DEBUG_ASSERT_PRECONDITION(
        gaia::db::client_t::s_session_context->txn_context->txn_log_offset.is_valid(),
        "Invalid txn log offset!");

    return get_txn_log_from_offset(gaia::db::client_t::s_session_context->txn_context->txn_log_offset);
}

static gaia::db::gaia_txn_id_t get_txn_id()
{
    return gaia::db::client_t::s_session_context->txn_context->txn_id;
}

static const gaia::db::caches::db_caches_t* get_db_caches()
{
    return gaia::db::client_t::s_session_context->db_caches.get();
}
