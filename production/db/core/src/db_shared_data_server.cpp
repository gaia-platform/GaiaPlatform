/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "db_server.hpp"
#include "db_shared_data.hpp"

gaia::db::locators_t* gaia::db::get_locators()
{
    // The server's locator segment should always be mapped whenever any callers
    // of this method are able to observe it.
    ASSERT_PRECONDITION(gaia::db::server_t::s_shared_locators.is_set(), "Server locators segment is unmapped!");
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

gaia::db::id_index_t* gaia::db::get_id_index()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the id_index segment unmapped).
    ASSERT_PRECONDITION(gaia::db::server_t::s_shared_id_index.is_set(), "Server id_index segment is unmapped!");
    return gaia::db::server_t::s_shared_id_index.data();
}

gaia::db::memory_manager::address_offset_t gaia::db::allocate_object(
    gaia_locator_t locator,
    size_t size)
{
    return gaia::db::server_t::allocate_object(locator, size);
}

gaia::db::index::indexes_t* gaia::db::get_indexes()
{
    return &gaia::db::server_t::s_shared_indexes;
}
