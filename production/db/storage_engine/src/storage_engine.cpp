/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "storage_engine.hpp"

thread_local int gaia::db::se_base::s_session_socket = -1;
thread_local gaia::db::gaia_xid_t gaia::db::se_base::s_transaction_id = 0;
thread_local gaia::db::se_base::log* gaia::db::se_base::s_log = nullptr;
