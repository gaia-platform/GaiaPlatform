/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia/common.hpp"

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
    flight_t::insert_row(flight_number - 1, {});
    gaia_id_t flight_id = flight_t::insert_row(flight_number, {});
    flight_t::insert_row(flight_number + 1, {});
    flight_t::insert_row(flight_number + 2, {});
    txn.commit();

    ASSERT_EQ(flight_t::get(flight_id).return_passengers().size(), 0);

    gaia_id_t passenger_id = passenger_t::insert_row("Spock", "Vulcan", flight_number);
    txn.commit();

    ASSERT_EQ(flight_t::get(flight_id).return_passengers().size(), 1);
    ASSERT_EQ(flight_t::get(flight_id).return_passengers().begin()->gaia_id(), passenger_id);
    ASSERT_EQ(passenger_t::get(passenger_id).return_flight().gaia_id(), flight_id);

    passenger_id = passenger_t::insert_row("James", "Kirk", flight_number);
    txn.commit();

    ASSERT_EQ(flight_t::get(flight_id).return_passengers().size(), 2);
    ASSERT_EQ(passenger_t::get(passenger_id).return_flight().gaia_id(), flight_id);
}

TEST_F(auto_connect_test, child_update_connect)
{
    const int32_t flight_number = 1701;
    auto_transaction_t txn;
    gaia_id_t passenger_id = passenger_t::insert_row("Spock", "Vulcan", 0);
    gaia_id_t flight_id = flight_t::insert_row(flight_number, {});
    txn.commit();

    ASSERT_EQ(flight_t::get(flight_id).return_passengers().size(), 0);
    ASSERT_EQ(passenger_t::get(passenger_id).return_flight().gaia_id(), c_invalid_gaia_id);

    auto passenger_writer = passenger_t::get(passenger_id).writer();
    passenger_writer.return_flight_number = flight_number;
    passenger_writer.update_row();
    txn.commit();

    ASSERT_EQ(flight_t::get(flight_id).return_passengers().size(), 1);
    ASSERT_EQ(flight_t::get(flight_id).return_passengers().begin()->gaia_id(), passenger_id);
    ASSERT_EQ(passenger_t::get(passenger_id).return_flight().gaia_id(), flight_id);
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
    ASSERT_EQ(passenger_t::get(passenger_id).return_flight().gaia_id(), flight_id);

    auto passenger_writer = passenger_t::get(passenger_id).writer();
    passenger_writer.return_flight_number = nullopt;
    passenger_writer.update_row();
    txn.commit();

    ASSERT_EQ(flight_t::get(flight_id).return_passengers().size(), 0);
    ASSERT_EQ(passenger_t::get(passenger_id).return_flight().gaia_id(), c_invalid_gaia_id);
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
    ASSERT_EQ(passenger_t::get(passenger_id).return_flight().gaia_id(), enterprise_flight_id);

    auto passenger_writer = passenger_t::get(passenger_id).writer();
    passenger_writer.return_flight_number = voyager_flight_number;
    passenger_writer.update_row();
    txn.commit();

    ASSERT_EQ(flight_t::get(enterprise_flight_id).return_passengers().size(), 0);
    ASSERT_EQ(flight_t::get(voyager_flight_id).return_passengers().size(), 1);
    ASSERT_EQ(flight_t::get(voyager_flight_id).return_passengers().begin()->gaia_id(), passenger_id);
    ASSERT_EQ(passenger_t::get(passenger_id).return_flight().gaia_id(), voyager_flight_id);
}

TEST_F(auto_connect_test, parent_insert_connect)
{
    const int32_t flight_number = 1701;
    auto_transaction_t txn;
    gaia_id_t spock_id = passenger_t::insert_row("Spock", "Vulcan", flight_number);
    gaia_id_t kirk_id = passenger_t::insert_row("James", "Kirk", flight_number);
    txn.commit();

    gaia_id_t flight_id = flight_t::insert_row(flight_number, {});
    txn.commit();

    std::set<gaia_id_t> passenger_ids{spock_id, kirk_id};

    ASSERT_EQ(flight_t::get(flight_id).return_passengers().size(), 2);
    std::set<gaia_id_t> return_passenger_ids;
    for (const auto& return_passenger : flight_t::get(flight_id).return_passengers())
    {
        return_passenger_ids.insert(return_passenger.gaia_id());
    }
    ASSERT_EQ(passenger_ids, return_passenger_ids);
}

TEST_F(auto_connect_test, parent_update_disconnect)
{
    const int32_t flight_number = 1701;
    auto_transaction_t txn;
    gaia_id_t flight_id = flight_t::insert_row(flight_number, {});
    gaia_id_t spock_id = passenger_t::insert_row("Spock", "Vulcan", flight_number);
    gaia_id_t kirk_id = passenger_t::insert_row("James", "Kirk", flight_number);
    txn.commit();

    ASSERT_EQ(flight_t::get(flight_id).return_passengers().size(), 2);
    ASSERT_EQ(passenger_t::get(spock_id).return_flight().gaia_id(), flight_id);
    ASSERT_EQ(passenger_t::get(kirk_id).return_flight().gaia_id(), flight_id);

    auto flight_writer = flight_t::get(flight_id).writer();
    flight_writer.number = nullopt;
    flight_writer.update_row();
    txn.commit();

    ASSERT_EQ(flight_t::get(flight_id).return_passengers().size(), 0);
    ASSERT_EQ(passenger_t::get(spock_id).return_flight_number(), flight_number);
    ASSERT_EQ(passenger_t::get(kirk_id).return_flight_number(), flight_number);
}

TEST_F(auto_connect_test, parent_update_reconnect)
{
    const int32_t old_flight_number = 1;
    const int32_t new_flight_number = 1701;

    auto_transaction_t txn;

    gaia_id_t flight_id = flight_t::insert_row(old_flight_number, {});
    txn.commit();

    gaia_id_t spock_id = passenger_t::insert_row("Spock", "Vulcan", new_flight_number);
    gaia_id_t kirk_id = passenger_t::insert_row("James", "Kirk", new_flight_number);
    txn.commit();

    ASSERT_EQ(passenger_t::get(spock_id).return_flight().gaia_id(), c_invalid_gaia_id);
    ASSERT_EQ(passenger_t::get(kirk_id).return_flight().gaia_id(), c_invalid_gaia_id);
    ASSERT_EQ(flight_t::get(flight_id).return_passengers().size(), 0);

    auto flight_writer = flight_t::get(flight_id).writer();
    flight_writer.number = new_flight_number;
    flight_writer.update_row();
    txn.commit();

    std::set<gaia_id_t> passenger_ids{spock_id, kirk_id};

    ASSERT_EQ(flight_t::get(flight_id).return_passengers().size(), 2);
    std::set<gaia_id_t> return_passenger_ids;
    for (const auto& return_passenger : flight_t::get(flight_id).return_passengers())
    {
        return_passenger_ids.insert(return_passenger.gaia_id());
    }
    ASSERT_EQ(passenger_ids, return_passenger_ids);
}

TEST_F(auto_connect_test, delete_parent)
{
    const int32_t old_flight_number = 1;
    const int32_t new_flight_number = 1701;

    auto_transaction_t txn;

    gaia_id_t flight_id = flight_t::insert_row(old_flight_number, {});
    txn.commit();

    gaia_id_t spock_id = passenger_t::insert_row("Spock", "Vulcan", old_flight_number);
    gaia_id_t kirk_id = passenger_t::insert_row("James", "Kirk", old_flight_number);
    txn.commit();

    ASSERT_EQ(flight_t::get(flight_id).return_passengers().size(), 2);
    ASSERT_EQ(passenger_t::get(spock_id).return_flight().gaia_id(), flight_id);
    ASSERT_EQ(passenger_t::get(kirk_id).return_flight().gaia_id(), flight_id);

    // A referenced parent object in a value linked relationship can be deleted
    // without the 'force' option.
    ASSERT_NO_THROW(flight_t::delete_row(flight_id));
    txn.commit();

    // The (previously auto) connected objects will be disconnected as a result.
    ASSERT_FALSE(passenger_t::get(spock_id).return_flight());
    ASSERT_FALSE(passenger_t::get(kirk_id).return_flight());
    txn.commit();

    auto spock_writer = passenger_t::get(spock_id).writer();
    spock_writer.return_flight_number = new_flight_number;
    spock_writer.update_row();
    txn.commit();

    flight_id = flight_t::insert_row(new_flight_number, {});
    txn.commit();

    auto kirk_writer = passenger_t::get(kirk_id).writer();
    kirk_writer.return_flight_number = new_flight_number;
    kirk_writer.update_row();
    txn.commit();

    // The disconnected objects can and will be auto-connected to new parent
    // object(s) if the linked field values match.
    ASSERT_EQ(passenger_t::get(spock_id).return_flight().gaia_id(), flight_id);
    ASSERT_EQ(passenger_t::get(kirk_id).return_flight().gaia_id(), flight_id);
}
