/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "se_server.hpp"
#include "se_shared_data.hpp"

gaia::db::locators* gaia::db::get_shared_locators_ptr()
{
    // The server's locator segment should always be mapped.
    retail_assert(gaia::db::server::s_shared_locators, "Server locators segment is unmapped!");
    return gaia::db::server::s_shared_locators;
}

gaia::db::data* gaia::db::get_shared_data_ptr()
{
    return gaia::db::server::s_data;
}
