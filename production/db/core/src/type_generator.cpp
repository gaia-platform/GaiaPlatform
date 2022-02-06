/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "type_generator.hpp"

#include <memory>

#include "gaia_internal/db/db_object.hpp"

#include "db_helpers.hpp"
#include "db_server.hpp"

using namespace gaia::common;
using namespace gaia::db::storage;

namespace gaia
{
namespace db
{

type_generator_t::type_generator_t(gaia_type_t type, gaia_txn_id_t txn_id)
    : m_type(type), m_txn_id(txn_id), m_iterator(), m_is_initialized(false), m_manage_local_snapshot(false)
{
}

std::optional<gaia_id_t> type_generator_t::operator()()
{
    // The initialization of the record_list iterator should be done by the same thread
    // that executes the iteration.
    if (!m_is_initialized)
    {
        std::shared_ptr<record_list_t> record_list = record_list_manager_t::get()->get_record_list(m_type);
        record_list->start(m_iterator);

        // Initialize local snapshot, if needed.
        // Also, set the flag to clear the snapshot if we opened it.
        if (!server_t::s_local_snapshot_locators.is_set())
        {
            ASSERT_INVARIANT(server_t::s_txn_id.is_valid() == false, "Transaction id already set!");
            server_t::s_txn_id = m_txn_id;
            bool replay_logs = false;
            server_t::create_local_snapshot(replay_logs);
            m_manage_local_snapshot = true;
        }
        m_is_initialized = true;
    }

    // Find the next valid record locator.
    while (!m_iterator.at_end())
    {
        gaia_locator_t locator = record_list_t::get_record_data(m_iterator).locator;
        db_object_t* db_object = nullptr;

        // The record on which the iterator stopped may have been deleted by another thread
        // before we could read its value, so we have to check again that we have retrieved a valid value.
        if (locator.is_valid())
        {
            db_object = locator_to_ptr(locator);
        }

        // Whether we found a record or not, we need to advance the iterator.
        record_list_t::move_next(m_iterator);

        // If the record was found, return its id.
        if (db_object)
        {
            ASSERT_PRECONDITION(db_object->id.is_valid(), "Database object has an invalid gaia_id value!");
            return db_object->id;
        }
    }

    // Signal end of scan.
    return std::nullopt;
}

void type_generator_t::cleanup()
{
    if (m_manage_local_snapshot)
    {
        server_t::s_local_snapshot_locators.close();
        server_t::s_txn_id = c_invalid_gaia_txn_id;
    }
}

} // namespace db
} // namespace gaia
