/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_internal/db/db_types.hpp"

#include "chunk_manager.hpp"
#include "db_caches.hpp"
#include "db_internal_types.hpp"
#include "mapped_data.hpp"
#include "memory_manager.hpp"
#include "memory_types.hpp"
#include "type_index.hpp"

#ifdef GAIA_DB_SERVER
#include "db_server.hpp"
#else
#include "db_client.hpp"
#endif

namespace gaia
{
namespace db
{

#include "db_shared_data_interface.inc"

#ifdef GAIA_DB_SERVER
#include "db_shared_data_server.inc"
#else
#include "db_shared_data_client.inc"
#endif

} // namespace db
} // namespace gaia
