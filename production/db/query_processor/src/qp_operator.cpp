////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "qp_operator.hpp"

#include "gaia_internal/db/db_client_config.hpp"
#include "gaia_internal/db/index_builder.hpp"

#include "db_client.hpp"

using namespace gaia::db;

namespace gaia
{
namespace db
{
namespace query_processor
{

void db_client_proxy_t::verify_txn_active()
{
    db::client_t::verify_txn_active();
}

gaia_txn_id_t db_client_proxy_t::get_current_txn_id()
{
    verify_txn_active();
    return gaia::db::get_txn_id();
}

void db_client_proxy_t::update_local_indexes()
{
    // Update local indexes with our transaction's changes,
    // using the transaction log.
    bool allow_create_empty = true;
    txn_log_t* txn_log = gaia::db::get_txn_log();
    index::index_builder_t::update_indexes_from_txn_log(
        txn_log,
        client_t::s_session_context->txn_context->last_index_processed_log_count,
        client_t::s_session_context->session_options.skip_catalog_integrity_check,
        allow_create_empty);

    // Update our log processing watermark, so that future calls
    // will not reprocess the log records that we already processed so far.
    client_t::s_session_context->txn_context->last_index_processed_log_count = txn_log->record_count;
}

} // namespace query_processor
} // namespace db
} // namespace gaia
