/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "storage_engine.hpp"

const char* gaia::db::gaia_mem_base::SCH_MEM_OFFSETS = "gaia_mem_offsets";
const char* gaia::db::gaia_mem_base::SCH_MEM_DATA = "gaia_mem_data";
std::string gaia::db::gaia_mem_base::s_sch_mem_offsets;
std::string gaia::db::gaia_mem_base::s_sch_mem_data;
int gaia::db::gaia_mem_base::s_fd_offsets = 0;
int gaia::db::gaia_mem_base::s_fd_data = 0;
gaia::db::gaia_id_t gaia::db::gaia_mem_base::s_next_id = 1000;  // not starting at 0
bool gaia::db::gaia_mem_base::s_engine = false;
gaia::db::gaia_mem_base::offsets* gaia::db::gaia_mem_base::s_offsets = nullptr;
gaia::db::gaia_mem_base::data* gaia::db::gaia_mem_base::s_data = nullptr;
gaia::db::gaia_mem_base::log* gaia::db::gaia_mem_base::s_log = nullptr;
gaia::db::threadPool* gaia::db::gaia_mem_base::trigger_pool = nullptr; 
gaia::db::gaia_tx_hook gaia::db::s_tx_begin_hook = nullptr;
gaia::db::gaia_tx_hook gaia::db::s_tx_commit_hook = nullptr;
gaia::db::gaia_tx_hook gaia::db::s_tx_rollback_hook = nullptr;
gaia::common::f_commit_trigger_t gaia::db::gaia_mem_base::s_tx_commit_trigger = nullptr;
