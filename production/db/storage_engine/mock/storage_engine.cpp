/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "storage_engine.hpp"

const char* gaia::db::gaia_mem_base::SCH_MEM_OFFSETS = "gaia_mem_offsets";
const char* gaia::db::gaia_mem_base::SCH_MEM_DATA = "gaia_mem_data";
int gaia::db::gaia_mem_base::s_fd_offsets = 0;
int gaia::db::gaia_mem_base::s_fd_data = 0;
gaia::db::gaia_id_t gaia::db::gaia_mem_base::s_next_id = 1000;  // not starting at 0
bool gaia::db::gaia_mem_base::s_engine = false;
gaia::db::gaia_mem_base::offsets* gaia::db::gaia_mem_base::s_offsets = nullptr;
gaia::db::gaia_mem_base::data* gaia::db::gaia_mem_base::s_data = nullptr;
gaia::db::gaia_mem_base::log* gaia::db::gaia_mem_base::s_log = nullptr;
