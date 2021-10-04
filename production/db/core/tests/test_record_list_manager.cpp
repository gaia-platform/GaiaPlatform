/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include <gtest/gtest.h>

#include "gaia_internal/db/db_types.hpp"

#include "record_list_manager.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::storage;

TEST(storage, record_list_manager)
{
    constexpr gaia_locator_t c_starting_locator = 1000;
    constexpr gaia_locator_t c_count_locators = 10;

    constexpr gaia_type_t c_type_even = 1000;
    constexpr gaia_type_t c_type_odd = 1001;

    record_list_manager_t* record_list_manager = record_list_manager_t::get();

    // Add some locator values to our list.
    for (gaia_locator_t locator = c_starting_locator; locator < c_starting_locator + c_count_locators; locator++)
    {
        if (locator % 2 == 0)
        {
            shared_ptr<record_list_t> record_list = record_list_manager->get_record_list(c_type_even);
            cout << "Adding " << locator << " to even type record list" << endl;
            record_list->add(locator);
        }
        else
        {
            shared_ptr<record_list_t> record_list = record_list_manager->get_record_list(c_type_odd);
            cout << "Adding " << locator << " to odd type record list" << endl;
            record_list->add(locator);
        }
    }

    // Start an iteration over the even locator list.
    shared_ptr<record_list_t> record_list = record_list_manager->get_record_list(c_type_even);
    record_iterator_t iterator;
    gaia_locator_t locator;
    bool return_value = record_list->start(iterator);
    ASSERT_EQ(true, return_value);
    ASSERT_EQ(false, iterator.at_end());

    // Iterate over the list and verify content.
    do
    {
        locator = record_list_t::get_record_data(iterator).locator;
        cout << "Found " << locator << " in even type record list" << endl;
        ASSERT_EQ(0, locator % 2);
    } while (record_list_t::move_next(iterator));

    ASSERT_EQ(true, iterator.at_end());

    // Start a new iteration over the odd locator list.
    record_list = record_list_manager->get_record_list(c_type_odd);
    return_value = record_list->start(iterator);
    ASSERT_EQ(true, return_value);
    ASSERT_EQ(false, iterator.at_end());

    // Iterate over the list and verify content.
    do
    {
        locator = record_list_t::get_record_data(iterator).locator;
        cout << "Found " << locator << " in odd type record list" << endl;
        ASSERT_EQ(1, locator % 2);
    } while (record_list_t::move_next(iterator));

    ASSERT_EQ(true, iterator.at_end());
}
