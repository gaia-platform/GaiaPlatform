/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "db_server.hpp"
#include "db_shared_data.hpp"

gaia::db::shared_locators_t* gaia::db::get_shared_locators()
{
    // The server's locator segment should always be mapped whenever any callers
    // of this method are able to observe it.
    retail_assert(gaia::db::server::s_locators.data(), "Server locators segment is unmapped!");
    return gaia::db::server::s_locators.data();
}

gaia::db::shared_counters_t* gaia::db::get_shared_counters()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the counters segment unmapped).
    retail_assert(gaia::db::server::s_counters.data(), "Server counters segment is unmapped!");
    return gaia::db::server::s_counters.data();
}

gaia::db::shared_data_t* gaia::db::get_shared_data()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the data segment unmapped).
    retail_assert(gaia::db::server::s_data.data(), "Server data segment is unmapped!");
    return gaia::db::server::s_data.data();
}

gaia::db::shared_id_index_t* gaia::db::get_shared_id_index()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the id_index segment unmapped).
    retail_assert(gaia::db::server::s_id_index.data(), "Server id_index segment is unmapped!");
    return gaia::db::server::s_id_index.data();
}

gaia::db::memory_manager::address_offset_t gaia::db::allocate_object(
    gaia_locator_t locator,
    size_t size)
{
    return gaia::db::server::allocate_object(locator, size);
}
