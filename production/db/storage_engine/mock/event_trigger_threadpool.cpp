/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "event_trigger_threadpool.hpp"
#include "wait_queue.hpp"

using namespace gaia::db;
using namespace gaia::db::triggers;

commit_trigger_fn event_trigger_threadpool_t::s_tx_commit_trigger = nullptr;
thread_local bool event_trigger_threadpool_t::session_active = false;
thread_local wait_queue_t<std::function<void()>> event_trigger_threadpool_t::tasks{};
