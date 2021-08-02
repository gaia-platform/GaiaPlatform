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
namespace query_processor
{

void physical_operator_t::verify_txn_active()
{
    db::client_t::verify_txn_active();
}

gaia_txn_id_t physical_operator_t::get_current_txn_id()
{
    verify_txn_active();
    return client_t::s_txn_id;
}

void physical_operator_t::rebuild_local_indexes()
{
    // Clear the indexes.
    for (const auto& idx : client_t::s_local_indexes)
    {
        idx.second->clear();
    }

    index::index_builder_t::update_indexes_from_logs(*client_t::s_log.data(), client_t::s_session_options.skip_catalog_integrity_check);
}

} // namespace query_processor
} // namespace gaia
