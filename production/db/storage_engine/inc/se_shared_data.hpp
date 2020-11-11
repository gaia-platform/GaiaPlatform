/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "se_types.hpp"

namespace gaia
{
namespace db
{

gaia::db::locators* get_shared_locators_ptr();

gaia::db::data* get_shared_data_ptr();

} // namespace db
} // namespace gaia
