/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "gaia_system.hpp"
#include "event_manager.hpp"
#include "storage_engine.hpp"


void gaia::system::initialize(bool is_engine)
{
    // create the singleton for the event manager
    gaia::rules::event_manager_t::get();

    // create the storage engine
    gaia::db::gaia_mem_base::init(is_engine);
}
