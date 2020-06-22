/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "gaia_system.hpp"
#include "gaia_catalog.hpp"
#include "rules.hpp"
#include "storage_engine.hpp"


void gaia::system::initialize(bool is_engine)
{
    // Create the storage engine first as the event manager depends on it.
    gaia::db::gaia_mem_base::init(is_engine);

    gaia::rules::initialize_rules_engine();
}
