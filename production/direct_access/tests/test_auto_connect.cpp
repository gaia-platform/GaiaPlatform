/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <string>

#include "gtest/gtest.h"

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_airport.h"

using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::airport;

class auto_connect_test : public db_catalog_test_base_t
{
protected:
    auto_connect_test()
        : db_catalog_test_base_t("airport.ddl"){};
};

TEST_F(auto_connect_test, child_insert_connect)
{
    const int32_t flight_number = 1701;
    auto_transaction_t txn;
    gaia_id_t flight_id = flight_t::insert_row(flight_number, {});
    txn.commit();

    ASSERT_EQ(flight_t::get(flight_id).return_passengers().size(), 0);

    gaia_id_t passenger_id = passenger_t::insert_row("Spock", "Vulcan", flight_number);
    txn.commit();

    ASSERT_EQ(flight_t::get(flight_id).return_passengers().size(), 1);
    ASSERT_EQ(flight_t::get(flight_id).return_passengers().begin()->gaia_id(), passenger_id);
}

TEST_F(auto_connect_test, child_update_connect)
{
    const int32_t flight_number = 1701;
    auto_transaction_t txn;
    gaia_id_t passenger_id = passenger_t::insert_row("Spock", "Vulcan", 0);
    gaia_id_t flight_id = flight_t::insert_row(flight_number, {});
    txn.commit();

    ASSERT_EQ(flight_t::get(flight_id).return_passengers().size(), 0);

    auto passenger_writer = passenger_t::get(passenger_id).writer();
    passenger_writer.return_flight_number = flight_number;
    passenger_writer.update_row();
    txn.commit();

    ASSERT_EQ(flight_t::get(flight_id).return_passengers().size(), 1);
    ASSERT_EQ(flight_t::get(flight_id).return_passengers().begin()->gaia_id(), passenger_id);
}

TEST_F(auto_connect_test, child_update_disconnect)
{
    const int32_t flight_number = 1701;
    auto_transaction_t txn;
    gaia_id_t flight_id = flight_t::insert_row(flight_number, {});
    gaia_id_t passenger_id = passenger_t::insert_row("Spock", "Vulcan", flight_number);
    txn.commit();

    ASSERT_EQ(flight_t::get(flight_id).return_passengers().size(), 1);
    ASSERT_EQ(flight_t::get(flight_id).return_passengers().begin()->gaia_id(), passenger_id);

    auto passenger_writer = passenger_t::get(passenger_id).writer();
    passenger_writer.return_flight_number = 0;
    passenger_writer.update_row();
    txn.commit();

    ASSERT_EQ(flight_t::get(flight_id).return_passengers().size(), 0);
}

TEST_F(auto_connect_test, child_update_reconnect)
{
    const int32_t enterprise_flight_number = 1701;
    const int32_t voyager_flight_number = 74656;

    auto_transaction_t txn;

    gaia_id_t enterprise_flight_id = flight_t::insert_row(enterprise_flight_number, {});
    gaia_id_t voyager_flight_id = flight_t::insert_row(voyager_flight_number, {});
    txn.commit();

    gaia_id_t passenger_id = passenger_t::insert_row("Spock", "Vulcan", enterprise_flight_number);
    txn.commit();

    ASSERT_EQ(flight_t::get(voyager_flight_id).return_passengers().size(), 0);
    ASSERT_EQ(flight_t::get(enterprise_flight_id).return_passengers().size(), 1);
    ASSERT_EQ(flight_t::get(enterprise_flight_id).return_passengers().begin()->gaia_id(), passenger_id);

    auto passenger_writer = passenger_t::get(passenger_id).writer();
    passenger_writer.return_flight_number = voyager_flight_number;
    passenger_writer.update_row();
    txn.commit();

    ASSERT_EQ(flight_t::get(enterprise_flight_id).return_passengers().size(), 0);
    ASSERT_EQ(flight_t::get(voyager_flight_id).return_passengers().size(), 1);
    ASSERT_EQ(flight_t::get(voyager_flight_id).return_passengers().begin()->gaia_id(), passenger_id);
}
