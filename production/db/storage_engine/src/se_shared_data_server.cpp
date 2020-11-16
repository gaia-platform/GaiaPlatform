/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "se_server.hpp"
#include "se_shared_data.hpp"

gaia::db::locators* gaia::db::get_shared_locators()
{
    // The server's locator segment should always be mapped whenever any callers
    // of this method are able to observe it.
    retail_assert(gaia::db::server::s_shared_locators, "Server locators segment is unmapped!");
    return gaia::db::server::s_shared_locators;
}

gaia::db::data* gaia::db::get_shared_data()
{
    // Since we don't use this accessor in the server itself, we can assert that
    // it is always non-null (since callers should never be able to see it in
    // its null state, i.e., with the data segment unmapped).
    retail_assert(gaia::db::server::s_data, "Server data segment is unmapped!");
    return gaia::db::server::s_data;
}
