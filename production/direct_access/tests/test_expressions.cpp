/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <ctime>

#include <iostream>
#include <string>

#include <gtest/gtest.h>

#include "gaia/common.hpp"
#include "gaia/direct_access/auto_transaction.hpp"

#include "gaia_internal/common/timer.hpp"
#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_addr_book.h"

using namespace gaia::addr_book;
using namespace gaia::addr_book::address_expr;
using namespace gaia::addr_book::employee_expr;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::direct_access;
using namespace gaia::db;
using namespace std;

using g_timer_t = gaia::common::timer_t;

class test_expressions : public db_catalog_test_base_t
{
public:
    test_expressions()
        : db_catalog_test_base_t("addr_book.ddl"){};

protected:
    address_t seattle, aberdeen, tyngsborough, puyallup, renton, bellevue, redmond, kissimmee;
    employee_t simone, dax, bill, laurentiu, wayne, yiwen, mihir, tobin;
    phone_t landline, mobile;

    void SetUp() override
    {
        db_catalog_test_base_t::SetUp();

        begin_transaction();

        seattle = create_address("Seattle", "WA");
        aberdeen = create_address("Aberdeen", "WA");
        tyngsborough = create_address("Tyngsborough", "MA");
        puyallup = create_address("Puyallup", "WA");
        renton = create_address("Renton", "WA");
        bellevue = create_address("Bellevue", "WA");
        redmond = create_address("Redmond", "WA");
        kissimmee = create_address("Kissimmee", "FL");

        dax = create_employee("Dax", "Hawkins", "dax@gaia.io", date(2019, 11, 15), aberdeen);
        bill = create_employee("Bill", "Clinton", "bill@gaia.io", date(2019, 11, 20), tyngsborough);
        wayne = create_employee("Wayne", "Warren", "wayne@personal.com", date(2020, 1, 10), puyallup);
        tobin = create_employee("Tobin", "Baker", "tobin@gaia.io", date(2020, 3, 10), renton);
        laurentiu = create_employee("Laurentiu", "Cristofor", "laurentiu@gaia.io", date(2020, 4, 1), bellevue);
        yiwen = create_employee("Yi Wen", "Wong", "yiwen@gaia.io", date(2020, 5, 10), seattle);
        mihir = create_employee("Mihir", "Jadhav", "mihir@gaia.io", date(2020, 5, 31), redmond);
        simone = create_employee("Simone", "Rondelli", "simone@gaia.io", date(2020, 7, 31), kissimmee);

        landline = create_phone("(206)867-5309", "landline", aberdeen);
        mobile = create_phone("(407) 123-4567", "mobile", kissimmee);

        commit_transaction();
    }

    employee_t create_employee(const string& name, const string& last_name, const string& email, int64_t hire_date, address_t& address)
    {
        auto employee_w = employee_writer();
        employee_w.name_first = name;
        employee_w.name_last = last_name;
        employee_w.email = email;
        employee_w.hire_date = hire_date;
        gaia_id_t id = employee_w.insert_row();
        employee_t employee = employee_t::get(id);

        employee.addressee_address_list().insert(address);

        return employee;
    }

    address_t create_address(const string& city, const string& state)
    {
        auto address_w = address_writer();
        address_w.city = city;
        address_w.state = state;
        return address_t::get(address_w.insert_row());
    }

    phone_t create_phone(const string& number, const string& type, address_t& address)
    {
        phone_writer writer;
        writer.phone_number = number;
        writer.type = type;
        phone_t phone = phone_t::get(writer.insert_row());

        address.phone_list().insert(phone);

        return phone;
    }

    /**
     * Return the given date in milliseconds since epoch.
     */
    int64_t date(int yyyy, int mm, int dd)
    {
        std::tm tm{
            .tm_mday = dd,
            .tm_mon = mm - 1,
            .tm_year = yyyy - 1900,
        };
        return mktime(&tm) * 1000;
    }

    template <class T_container, class T_type>
    void assert_contains(T_container gaia_container, std::initializer_list<T_type> expected)
    {
        auto expected_vec = std::vector(expected);

        ASSERT_EQ(expected.size(), std::distance(gaia_container.begin(), gaia_container.end()));

        for (const T_type& obj : gaia_container)
        {
            auto position = std::find(expected_vec.begin(), expected_vec.end(), obj);

            if (position != expected_vec.end())
            {
                expected_vec.erase(position);
            }
        }

        ASSERT_TRUE(expected_vec.empty());
    }

    template <class T_container, class T_type>
    void assert_contains(T_container gaia_container, T_type expected)
    {
        assert_contains(gaia_container, {expected});
    }

    template <class T_container>
    void assert_empty(T_container gaia_container)
    {
        ASSERT_EQ(gaia_container.begin(), gaia_container.end());
    }
};

TEST_F(test_expressions, gaia_id_ed)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::gaia_id == yiwen.gaia_id()),
        yiwen);

    assert_empty(
        employee_t::list()
            .where(employee_t::expr::gaia_id == seattle.gaia_id()));
}

TEST_F(test_expressions, int64_eq)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(hire_date == date(2020, 5, 10)),
        {yiwen});

    assert_empty(
        employee_t::list()
            .where(hire_date == date(2050, 5, 10)));
}

TEST_F(test_expressions, int64_ne)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(hire_date != date(2020, 5, 10)),
        {simone, mihir, laurentiu, tobin, wayne, bill, dax});

    assert_contains(
        employee_t::list()
            .where(hire_date != date(2050, 5, 10)),
        {simone, mihir, yiwen, laurentiu, tobin, wayne, bill, dax});
}

TEST_F(test_expressions, int64_gt)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(hire_date > date(2020, 5, 10)),
        {simone, mihir});

    assert_empty(
        employee_t::list()
            .where(hire_date > date(2050, 5, 10)));
}

TEST_F(test_expressions, int64_gteq)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(hire_date >= date(2020, 5, 10)),
        {simone, mihir, yiwen});

    assert_empty(
        employee_t::list()
            .where(hire_date >= date(2050, 5, 10)));
}

TEST_F(test_expressions, int64_lt)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(hire_date < date(2020, 5, 10)),
        {laurentiu, tobin, wayne, bill, dax});

    assert_empty(
        employee_t::list()
            .where(hire_date < date(1902, 5, 10)));
}

TEST_F(test_expressions, int64_lteq)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(hire_date <= date(2020, 5, 10)),
        {yiwen, laurentiu, tobin, wayne, bill, dax});

    assert_empty(
        employee_t::list()
            .where(hire_date <= date(1902, 5, 10)));
}

TEST_F(test_expressions, string_eq)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(name_first == "Simone"),
        simone);

    assert_contains(
        employee_t::list()
            .where(name_first == std::string("Simone")),
        simone);

    const char* surname = "Hawkins";
    assert_contains(
        employee_t::list()
            .where(name_last == surname),
        dax);

    assert_empty(
        employee_t::list()
            .where(name_first == "simone"));

    assert_empty(
        employee_t::list()
            .where(name_first == "Olbudio"));
}

TEST_F(test_expressions, string_ne)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(name_first != "Simone"),
        {dax, bill, laurentiu, wayne, yiwen, mihir, tobin});

    assert_contains(
        employee_t::list()
            .where(name_first != std::string("Simone")),
        {dax, bill, laurentiu, wayne, yiwen, mihir, tobin});

    const char* surname = "Hawkins";
    assert_contains(
        employee_t::list()
            .where(name_last != surname),
        {simone, bill, laurentiu, wayne, yiwen, mihir, tobin});

    assert_contains(
        employee_t::list()
            .where(name_first != "Olbudio"),
        {simone, dax, bill, laurentiu, wayne, yiwen, mihir, tobin});
}

TEST_F(test_expressions, object_eq)
{
    auto_transaction_t txn;

    assert_contains(
        address_t::list()
            .where(addressee_employee == yiwen),
        seattle);

    auto employee_writer = gaia::addr_book::employee_writer();
    employee_writer.name_first = "Zack";
    employee_writer.name_last = "Nolan";
    auto zack = employee_t::get(employee_writer.insert_row());

    assert_empty(
        address_t::list()
            .where(addressee_employee == zack));

    assert_empty(
        address_t::list()
            .where(addressee_employee == employee_t()));
}

TEST_F(test_expressions, object_ne)
{
    auto_transaction_t txn;

    assert_contains(
        address_t::list()
            .where(addressee_employee != yiwen),
        {aberdeen, tyngsborough, puyallup, renton, bellevue, redmond, kissimmee});

    auto employee_writer = gaia::addr_book::employee_writer();
    employee_writer.name_first = "Zack";
    employee_writer.name_last = "Nolan";
    auto zack = employee_t::get(employee_writer.insert_row());

    assert_contains(
        address_t::list()
            .where(addressee_employee != zack),
        {seattle, aberdeen, tyngsborough, puyallup, renton, bellevue, redmond, kissimmee});

    assert_contains(
        address_t::list()
            .where(addressee_employee != employee_t()),
        {seattle, aberdeen, tyngsborough, puyallup, renton, bellevue, redmond, kissimmee});
}

TEST_F(test_expressions, or_predicate)
{
    auto_transaction_t txn;

    auto employees = employee_t::list().where(
        name_first == "Wayne"
        || name_first == "Bill"
        || name_first == "Cristofor");

    assert_contains(employees, {wayne, bill});

    employees = employee_t::list().where(
        hire_date <= date(2020, 1, 10)
        || hire_date >= date(2020, 5, 31)
        || name_last == "Cristofor");

    assert_contains(employees, {dax, bill, wayne, laurentiu, simone, mihir});

    employees = employee_t::list().where(
        hire_date <= date(1991, 1, 1)
        || hire_date >= date(2036, 2, 7));

    assert_empty(employees);
}

TEST_F(test_expressions, and_expr)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(name_first == "Wayne" && name_last == "Warren"),
        wayne);

    assert_contains(
        employee_t::list()
            .where(hire_date >= date(2019, 11, 20) && name_last == "Clinton"),
        bill);

    assert_empty(
        employee_t::list()
            .where(hire_date <= date(2021, 1, 1) && hire_date >= date(2036, 2, 7)));
}

TEST_F(test_expressions, not_expr)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(!(name_first == "Wayne") && !(name_last == "Warren")),
        {simone, dax, bill, laurentiu, yiwen, mihir, tobin});

    assert_contains(
        employee_t::list()
            .where(!(hire_date >= date(2020, 1, 1))),
        {bill, dax});

    assert_empty(
        employee_t::list()
            .where(!(hire_date > date(2001, 1, 1))));
}

TEST_F(test_expressions, mix_boolean_expr)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where((name_first == "Wayne" && name_last == "Warren") && hire_date < date(2036, 2, 7)),
        wayne);

    assert_contains(
        employee_t::list()
            .where((name_first == "Wayne" && name_last == "Warren") || (hire_date > date(2036, 2, 7) || hire_date == date(2019, 11, 15))),
        {wayne, dax});

    assert_empty(
        employee_t::list()
            .where((name_first == "Wayne" && name_last == "Warren") && (hire_date > date(2036, 2, 7))));
}

TEST_F(test_expressions, container_contains_predicate)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(addressee_address_list
                       .contains(address_expr::state == "WA")),
        {dax, wayne, tobin, laurentiu, yiwen, mihir});

    assert_empty(
        employee_t::list()
            .where(addressee_address_list
                       .contains(address_expr::state == "CA")));
}

TEST_F(test_expressions, container_contains_object)
{
    auto_transaction_t txn;

    assert_contains(employee_t::list().where(addressee_address_list.contains(bellevue)), {laurentiu});

    auto marzabotto = create_address("Marzabotto", "IT");

    assert_empty(employee_t::list().where(addressee_address_list.contains(marzabotto)));
}

TEST_F(test_expressions, nested_container)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list().where(
            addressee_address_list.contains((address_expr::state == "WA" || address_expr::state == "FL") && address_t::expr::phone_list.contains(phone_expr::type == "landline"))),
        {dax});
}

TEST_F(test_expressions, container_empty)
{
    auto_transaction_t txn;

    assert_contains(
        address_t::list()
            // phone_list is ambiguous, need full qualification.
            .where(address_expr::phone_list.empty()),
        {seattle, tyngsborough, puyallup, renton, bellevue, redmond});

    assert_empty(
        employee_t::list()
            .where(addressee_address_list.empty()));
}

TEST_F(test_expressions, container_count)
{
    auto_transaction_t txn;

    assert_contains(
        address_t::list()
            .where(address_expr::phone_list.count() == 0),
        {seattle, tyngsborough, puyallup, renton, bellevue, redmond});

    assert_contains(
        employee_t::list()
            .where(addressee_address_list.count() >= 1),
        {simone, dax, bill, laurentiu, wayne, yiwen, mihir, tobin});

    assert_empty(
        employee_t::list()
            .where(addressee_address_list.count() > 10));
}
