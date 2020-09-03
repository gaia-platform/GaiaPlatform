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

}  // namespace db
}  // namespace gaia
