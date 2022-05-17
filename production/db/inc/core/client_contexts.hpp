/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>

#include "gaia_internal/db/db_client_config.hpp"
#include "gaia_internal/db/db_types.hpp"

#include "chunk_manager.hpp"
#include "db_caches.hpp"
#include "db_internal_types.hpp"
#include "memory_manager.hpp"

namespace gaia
{
namespace db
{

struct client_transaction_context_t
{
    gaia_txn_id_t txn_id;
    log_offset_t txn_log_offset;

    gaia::db::index::indexes_t local_indexes;

    // A log processing watermark that is used for index maintenance.
    size_t last_index_processed_log_count{0};

public:
    inline ~client_transaction_context_t();

    void clear();
};

struct client_session_context_t
{
    // The transaction context.
    std::shared_ptr<client_transaction_context_t> txn_context;

    config::session_options_t session_options;

    memory_manager::memory_manager_t memory_manager;
    memory_manager::chunk_manager_t chunk_manager;

    // Database caches are initialized for each session
    // during the first transaction of the session.
    std::shared_ptr<caches::db_caches_t> db_caches;

    // REVIEW [GAIAPLAT-2068]: When we enable snapshot reuse across txns (by
    // applying the undo log from the previous txn to the existing snapshot and
    // then applying redo logs from txns committed after the last shared
    // locators view update), we need to track the last commit_ts whose log was
    // applied to the snapshot, so we can ignore any logs committed at or before
    // that commit_ts.
    gaia_txn_id_t latest_applied_commit_ts;

public:
    inline ~client_session_context_t();

    void clear();
};

#include "client_contexts.inc"

} // namespace db
} // namespace gaia
