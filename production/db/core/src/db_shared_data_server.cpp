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
    common::retail_assert(gaia::db::server::s_shared_locators.is_initialized(), "Server locators segment is unmapped!");
    return gaia::db::server::s_shared_locators.data();
}

gaia::db::counters_t* gaia::db::get_counters()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the counters segment unmapped).
    common::retail_assert(gaia::db::server::s_shared_counters.is_initialized(), "Server counters segment is unmapped!");
    return gaia::db::server::s_shared_counters.data();
}

gaia::db::data_t* gaia::db::get_data()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the data segment unmapped).
    common::retail_assert(gaia::db::server::s_shared_data.is_initialized(), "Server data segment is unmapped!");
    return gaia::db::server::s_shared_data.data();
}

gaia::db::id_index_t* gaia::db::get_id_index()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the id_index segment unmapped).
    common::retail_assert(gaia::db::server::s_shared_id_index.is_initialized(), "Server id_index segment is unmapped!");
    return gaia::db::server::s_shared_id_index.data();
}

gaia::db::memory_manager::address_offset_t gaia::db::allocate_object(
    gaia_locator_t locator,
    size_t size)
{
    return gaia::db::server::allocate_object(locator, size);
}
