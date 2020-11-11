/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "se_client.hpp"
#include "se_shared_data.hpp"

gaia::db::locators* gaia::db::get_shared_locators_ptr()
{
    return gaia::db::client::s_locators;
}

gaia::db::data* gaia::db::get_shared_data_ptr()
{
    return gaia::db::client::s_data;
}
