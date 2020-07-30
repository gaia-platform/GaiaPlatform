/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "storage_engine.hpp"

const char* const gaia::db::se_base::SERVER_CONNECT_SOCKET_NAME = "gaia_se_server";
const char* const gaia::db::se_base::SCH_MEM_OFFSETS = "gaia_mem_offsets";
const char* const gaia::db::se_base::SCH_MEM_DATA = "gaia_mem_data";
const char* const gaia::db::se_base::SCH_MEM_LOG = "gaia_mem_log";

int gaia::db::se_base::s_fd_offsets = -1;
thread_local int gaia::db::se_base::s_session_socket = -1;
thread_local gaia::common::gaia_xid_t gaia::db::se_base::s_transaction_id = 0;
gaia::db::se_base::data* gaia::db::se_base::s_data = nullptr;
thread_local gaia::db::se_base::log* gaia::db::se_base::s_log = nullptr;
std::string gaia::db::se_base::s_server_socket_name(SERVER_CONNECT_SOCKET_NAME);
