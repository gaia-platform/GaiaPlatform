/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "db_client.hpp"
#include "se_shared_data.hpp"

gaia::db::locators_t* gaia::db::get_shared_locators()
{
    if (!gaia::db::client::s_locators)
    {
        throw no_open_transaction();
    }

    // REVIEW: Callers of this method should probably never be able to observe
    // the locators segment in an unmapped state (i.e., outside a transaction),
    // but asserting this condition currently breaks test code that assumes we
    // should instead throw a `transaction_not_open` exception. We need to
    // determine whether throwing an exception is even appropriate (since
    // observing a null locators pointer should always be the result of a
    // programming error, i.e., calling a transactional operation outside a
    // transaction) before adding an assert here.
    return gaia::db::client::s_locators;
}

gaia::db::shared_counters_t* gaia::db::get_shared_counters()
{
    // Since we don't use this accessor in the client itself, we can assert that
    // it is always non-null (since callers should never be able to observe it
    // in its null state, i.e., with the counters segment unmapped).

    if (!gaia::db::client::s_counters)
    {
        throw no_active_session();
    }

    return gaia::db::client::s_counters;
}

gaia::db::shared_data_t* gaia::db::get_shared_data()
{
    // Since we don't use this accessor in the client itself, we can assert that
    // it is always non-null (since callers should never be able to observe it
    // in its null state, i.e., with the data segment unmapped).

    if (!gaia::db::client::s_data)
    {
        throw no_active_session();
    }

    return gaia::db::client::s_data;
}

gaia::db::shared_id_index_t* gaia::db::get_shared_id_index()
{
    // Since we don't use this accessor in the client itself, we can assert that
    // it is always non-null (since callers should never be able to observe it
    // in its null state, i.e., with the id_index segment unmapped).

    if (!gaia::db::client::s_id_index)
    {
        throw no_active_session();
    }

    return gaia::db::client::s_id_index;
}

gaia::db::memory_manager::address_offset_t gaia::db::allocate_object(
    gaia_locator_t locator,
    gaia::db::memory_manager::address_offset_t old_slot_offset,
    size_t size)
{
    return gaia::db::client::allocate_object(locator, old_slot_offset, size);
}
