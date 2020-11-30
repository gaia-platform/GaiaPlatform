/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "se_client.hpp"
#include "se_shared_data.hpp"

gaia::db::locators* gaia::db::get_shared_locators()
{
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

gaia::db::data* gaia::db::get_shared_data()
{
    // Since we don't use this accessor in the client itself, we can assert that
    // it is always non-null (since callers should never be able to observe it
    // in its null state, i.e., with the data segment unmapped).
    retail_assert(gaia::db::client::s_data, "Client data segment is unmapped!");
    return gaia::db::client::s_data;
}

void gaia::db::allocate_object(
    gaia_locator_t locator,
    memory_manager::address_offset_t old_slot_offset,
    size_t size)
{
    gaia::db::client::allocate_object(locator, old_slot_offset, size);
}
