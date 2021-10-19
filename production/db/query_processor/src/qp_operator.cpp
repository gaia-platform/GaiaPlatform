/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

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
    return client_t::s_txn_id;
}

void db_client_proxy_t::rebuild_local_indexes()
{
    // Clear the indexes.
    for (const auto& index : client_t::s_local_indexes)
    {
        index.second->clear();
    }

    bool allow_create_empty = true;

    index::index_builder_t::update_indexes_from_logs(
        *client_t::s_log.data(), client_t::s_session_options.skip_catalog_integrity_check, allow_create_empty);
}

} // namespace query_processor
} // namespace db
} // namespace gaia
