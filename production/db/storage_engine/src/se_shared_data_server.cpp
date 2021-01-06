/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "se_server.hpp"
#include "se_shared_data.hpp"

gaia::db::locators_t* gaia::db::get_shared_locators()
{
    // The server's locator segment should always be mapped whenever any callers
    // of this method are able to observe it.
    retail_assert(gaia::db::server::s_shared_locators, "Server locators segment is unmapped!");
    return gaia::db::server::s_shared_locators;
}

gaia::db::shared_counters_t* gaia::db::get_shared_counters()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the counters segment unmapped).
    retail_assert(gaia::db::server::s_counters, "Server counters segment is unmapped!");
    return gaia::db::server::s_counters;
}

gaia::db::shared_data_t* gaia::db::get_shared_data()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the data segment unmapped).
    retail_assert(gaia::db::server::s_data, "Server data segment is unmapped!");
    return gaia::db::server::s_data;
}

gaia::db::shared_id_index_t* gaia::db::get_shared_id_index()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the id_index segment unmapped).
    retail_assert(gaia::db::server::s_id_index, "Server id_index segment is unmapped!");
    return gaia::db::server::s_id_index;
}

gaia::db::page_alloc_counts_t* gaia::db::get_shared_page_alloc_counts()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the id_index segment unmapped).
    retail_assert(
        gaia::db::server::s_page_alloc_counts,
        "Page allocation counts segment is unmapped!");
    return gaia::db::server::s_page_alloc_counts;
}
