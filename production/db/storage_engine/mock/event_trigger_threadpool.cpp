/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "event_trigger_threadpool.hpp"

using namespace gaia::db::triggers;

f_commit_trigger_t event_trigger_threadpool_t::s_tx_commit_trigger = nullptr;
thread_local bool event_trigger_threadpool_t::session_active = false;
