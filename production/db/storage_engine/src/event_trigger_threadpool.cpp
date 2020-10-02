/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "event_trigger_threadpool.hpp"

using namespace gaia::db::triggers;

commit_trigger_fn event_trigger_threadpool_t::s_txn_commit_trigger = nullptr;
thread_local bool event_trigger_threadpool_t::session_active = false;
