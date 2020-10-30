/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_system.hpp"

#include "catalog.hpp"
#include "gaia_db.hpp"
#include "logger.hpp"
#include "rules.hpp"

void gaia::system::initialize()
{
    // Create the storage engine session first as the event manager depends on it.
    gaia::db::begin_session();
    gaia::catalog::initialize_catalog();
    gaia::rules::initialize_rules_engine();
}
