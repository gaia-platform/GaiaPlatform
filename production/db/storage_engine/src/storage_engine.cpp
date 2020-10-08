/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "storage_engine.hpp"

const char* const gaia::db::se_base::SERVER_CONNECT_SOCKET_NAME = "gaia_se_server";
const char* const gaia::db::se_base::SCH_MEM_LOCATORS = "gaia_mem_locators";
const char* const gaia::db::se_base::SCH_MEM_DATA = "gaia_mem_data";
const char* const gaia::db::se_base::SCH_MEM_LOG = "gaia_mem_log";

thread_local int gaia::db::se_base::s_session_socket = -1;
thread_local gaia::db::gaia_txn_id_t gaia::db::se_base::s_transaction_id = 0;
thread_local gaia::db::se_base::log* gaia::db::se_base::s_log = nullptr;
