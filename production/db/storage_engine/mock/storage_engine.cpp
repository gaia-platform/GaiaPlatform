/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "storage_engine.hpp"

const char* const gaia::db::gaia_mem_base::SERVER_CONNECT_SOCKET_NAME = "gaia_se_server";
const char* const gaia::db::gaia_mem_base::SCH_MEM_OFFSETS = "gaia_mem_offsets";
const char* const gaia::db::gaia_mem_base::SCH_MEM_DATA = "gaia_mem_data";

int gaia::db::gaia_mem_base::s_fd_offsets = -1;
thread_local int gaia::db::gaia_mem_base::s_session_socket = -1;
gaia::db::gaia_mem_base::data* gaia::db::gaia_mem_base::s_data = nullptr;
thread_local gaia::db::gaia_mem_base::log* gaia::db::gaia_mem_base::s_log = nullptr;
