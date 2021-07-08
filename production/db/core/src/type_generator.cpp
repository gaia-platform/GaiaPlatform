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

type_generator_t::type_generator_t(gaia_type_t type)
    : m_type(type), m_iterator(), m_is_initialized(false)
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
        bool replay_logs = false;
        server_t::create_local_snapshot(replay_logs);
        m_is_initialized = true;
    }

    // Find the next valid record locator.
    while (!m_iterator.at_end())
    {
        gaia_locator_t locator = record_list_t::get_record_data(m_iterator).locator;
        ASSERT_INVARIANT(
            locator != c_invalid_gaia_locator, "An invalid locator value was returned from record list iteration!");

        db_object_t* db_object = locator_to_ptr(locator);

        // Whether we found a record or not, we need to advance the iterator.
        record_list_t::move_next(m_iterator);

        // If the record was found, return its id.
        if (db_object)
        {
            return db_object->id;
        }
    }

    // Signal end of scan.
    return std::nullopt;
}

type_generator_t::~type_generator_t()
{
    if (m_is_initialized)
    {
        server_t::s_local_snapshot_locators.close();
    }
}

} // namespace db
} // namespace gaia
