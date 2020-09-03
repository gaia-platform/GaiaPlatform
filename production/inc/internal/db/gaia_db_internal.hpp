/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once
#include "triggers.hpp"

namespace gaia {
namespace db {

// Set by the rules engine
extern triggers::commit_trigger_fn s_tx_commit_trigger;

void clear_shared_memory();

// Todo (Mihir): Expose options to set the persistent 
// directory path & also some way to destroy it instead 
// of hardcoding the path. 
// https://gaiaplatform.atlassian.net/browse/GAIAPLAT-310
const char* const PERSISTENT_DIRECTORY_PATH = "/tmp/gaia_db";

}  // namespace db
}  // namespace gaia
