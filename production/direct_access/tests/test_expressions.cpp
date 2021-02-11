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

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_addr_book.h"

using namespace gaia::addr_book;
using namespace gaia::common;
using namespace gaia::direct_access;
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
        bill = create_employee("Bill", "Clinton", "dax@gaia.io", date(2019, 11, 20), tyngsborough);
        wayne = create_employee("Wayne", "Warren", "wayne@personal.com", date(2020, 1, 10), puyallup);
        tobin = create_employee("Tobin", "Baker", "tobin@gaia.io", date(2020, 3, 10), renton);
        laurentiu = create_employee("Laurentiu", "Cristofor", "laurentiu@gaia.io", date(2020, 4, 1), bellevue);
        yiwen = create_employee("Yi Wen", "Wong", "yiwen@gaia.io", date(2020, 5, 10), seattle);
        mihir = create_employee("Mihir", "Jadhav", "mihir@gaia.io", date(2020, 5, 31), redmond);
        simone = create_employee("Simone", "Rondelli", "simone@gaia.io", date(2020, 7, 31), kissimmee);

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

TEST_F(test_expressions, eq)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::hire_date == date(2020, 5, 10)),
        {yiwen});

    assert_empty(
        employee_t::list()
            .where(employee_t::expr::hire_date == date(2050, 5, 10)));

    txn.commit();
}

TEST_F(test_expressions, ne)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::hire_date != date(2020, 5, 10)),
        {simone, mihir, laurentiu, tobin, wayne, bill, dax});

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::hire_date != date(2050, 5, 10)),
        {simone, mihir, yiwen, laurentiu, tobin, wayne, bill, dax});

    txn.commit();
}

TEST_F(test_expressions, gt)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::hire_date > date(2020, 5, 10)),
        {simone, mihir});

    assert_empty(
        employee_t::list()
            .where(employee_t::expr::hire_date > date(2050, 5, 10)));

    txn.commit();
}

TEST_F(test_expressions, gteq)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::hire_date >= date(2020, 5, 10)),
        {simone, mihir, yiwen});

    assert_empty(
        employee_t::list()
            .where(employee_t::expr::hire_date >= date(2050, 5, 10)));

    txn.commit();
}

TEST_F(test_expressions, lt)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::hire_date < date(2020, 5, 10)),
        {laurentiu, tobin, wayne, bill, dax});

    assert_empty(
        employee_t::list()
            .where(employee_t::expr::hire_date < date(1902, 5, 10)));

    txn.commit();
}

TEST_F(test_expressions, lteq)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::hire_date <= date(2020, 5, 10)),
        {yiwen, laurentiu, tobin, wayne, bill, dax});

    assert_empty(
        employee_t::list()
            .where(employee_t::expr::hire_date <= date(1902, 5, 10)));

    txn.commit();
}

TEST_F(test_expressions, string_eq)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::name_first == "Simone"),
        simone);

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::name_first == std::string("Simone")),
        simone);

    const char* surname = "Hawkins";
    assert_contains(
        employee_t::list()
            .where(employee_t::expr::name_last == surname),
        dax);

    assert_empty(
        employee_t::list()
            .where(employee_t::expr::name_first == "simone"));

    assert_empty(
        employee_t::list()
            .where(employee_t::expr::name_first == "Olbudio"));

    txn.commit();
}

TEST_F(test_expressions, string_eq_case_insensitive)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::name_first.equals("simone", string_comparison_t::case_insensitive)),
        simone);

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::name_first.equals(std::string("simone"), string_comparison_t::case_insensitive)),
        simone);

    const char* surname = "hawkins";
    assert_contains(
        employee_t::list()
            .where(employee_t::expr::name_last.equals(surname, string_comparison_t::case_insensitive)),
        dax);

    assert_empty(
        employee_t::list()
            .where(employee_t::expr::name_first == "olbudio"));

    txn.commit();
}

TEST_F(test_expressions, string_ne)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::name_first != "Simone"),
        {dax, bill, laurentiu, wayne, yiwen, mihir, tobin});

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::name_first != std::string("Simone")),
        {dax, bill, laurentiu, wayne, yiwen, mihir, tobin});

    const char* surname = "Hawkins";
    assert_contains(
        employee_t::list()
            .where(employee_t::expr::name_last != surname),
        {simone, bill, laurentiu, wayne, yiwen, mihir, tobin});

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::name_first != "Olbudio"),
        {simone, dax, bill, laurentiu, wayne, yiwen, mihir, tobin});

    txn.commit();
}

TEST_F(test_expressions, object_eq)
{
    auto_transaction_t txn;

    assert_contains(
        address_t::list()
            .where(address_t::expr::addressee_employee == yiwen),
        seattle);

    auto employee_writer = gaia::addr_book::employee_writer();
    employee_writer.name_first = "Zack";
    employee_writer.name_last = "Nolan";
    auto zack = employee_t::get(employee_writer.insert_row());

    assert_empty(
        address_t::list()
            .where(address_t::expr::addressee_employee == zack));

    assert_empty(
        address_t::list()
            .where(address_t::expr::addressee_employee == employee_t()));

    txn.commit();
}

TEST_F(test_expressions, object_ne)
{
    auto_transaction_t txn;

    assert_contains(
        address_t::list()
            .where(address_t::expr::addressee_employee != yiwen),
        {aberdeen, tyngsborough, puyallup, renton, bellevue, redmond, kissimmee});

    auto employee_writer = gaia::addr_book::employee_writer();
    employee_writer.name_first = "Zack";
    employee_writer.name_last = "Nolan";
    auto zack = employee_t::get(employee_writer.insert_row());

    assert_contains(
        address_t::list()
            .where(address_t::expr::addressee_employee != zack),
        {seattle, aberdeen, tyngsborough, puyallup, renton, bellevue, redmond, kissimmee});

    assert_contains(
        address_t::list()
            .where(address_t::expr::addressee_employee != employee_t()),
        {seattle, aberdeen, tyngsborough, puyallup, renton, bellevue, redmond, kissimmee});

    txn.commit();
}

TEST_F(test_expressions, or_)
{
    auto_transaction_t txn;

    auto employees = employee_t::list().where(
        employee_t::expr::name_first == "Wayne"
        || employee_t::expr::name_first == "Bill"
        || employee_t::expr::name_first == "Cristofor");

    assert_contains(employees, {wayne, bill});

    employees = employee_t::list().where(
        employee_t::expr::hire_date <= date(2020, 1, 10)
        || employee_t::expr::hire_date >= date(2020, 5, 31)
        || employee_t::expr::name_last == "Cristofor");

    assert_contains(employees, {dax, bill, wayne, laurentiu, simone, mihir});

    employees = employee_t::list().where(
        employee_t::expr::hire_date <= date(1991, 1, 1)
        || employee_t::expr::hire_date >= date(2036, 2, 7));

    assert_empty(employees);

    txn.commit();
}

TEST_F(test_expressions, and_)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(
                employee_t::expr::name_first == "Wayne"
                && employee_t::expr::name_last == "Warren"),
        wayne);

    assert_contains(
        employee_t::list()
            .where(
                employee_t::expr::hire_date >= date(2019, 11, 20)
                && employee_t::expr::name_last == "Clinton"),
        bill);

    assert_empty(
        employee_t::list()
            .where(
                employee_t::expr::hire_date <= date(2021, 1, 1)
                && employee_t::expr::hire_date >= date(2036, 2, 7)));

    txn.commit();
}

TEST_F(test_expressions, not_)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(
                !(employee_t::expr::name_first == "Wayne")
                && !(employee_t::expr::name_last == "Warren")),
        {simone, dax, bill, laurentiu, yiwen, mihir, tobin});

    assert_contains(
        employee_t::list()
            .where(
                !(employee_t::expr::hire_date >= date(2020, 1, 1))),
        {bill, dax});

    assert_empty(
        employee_t::list()
            .where(
                !(employee_t::expr::hire_date > date(2001, 1, 1))));

    txn.commit();
}
