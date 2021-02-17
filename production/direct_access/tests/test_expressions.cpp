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
        bill = create_employee("Bill", "Clinton", "bill@gaia.io", date(2019, 11, 20), tyngsborough);
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
            .where(employee_t::expr::hire_date == date(2020, 5, 10)),
        {yiwen});

    assert_empty(
        employee_t::list()
            .where(employee_t::expr::hire_date == date(2050, 5, 10)));
}

TEST_F(test_expressions, int64_ne)
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
}

TEST_F(test_expressions, int64_gt)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::hire_date > date(2020, 5, 10)),
        {simone, mihir});

    assert_empty(
        employee_t::list()
            .where(employee_t::expr::hire_date > date(2050, 5, 10)));
}

TEST_F(test_expressions, int64_gteq)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::hire_date >= date(2020, 5, 10)),
        {simone, mihir, yiwen});

    assert_empty(
        employee_t::list()
            .where(employee_t::expr::hire_date >= date(2050, 5, 10)));
}

TEST_F(test_expressions, int64_lt)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::hire_date < date(2020, 5, 10)),
        {laurentiu, tobin, wayne, bill, dax});

    assert_empty(
        employee_t::list()
            .where(employee_t::expr::hire_date < date(1902, 5, 10)));
}

TEST_F(test_expressions, int64_lteq)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::hire_date <= date(2020, 5, 10)),
        {yiwen, laurentiu, tobin, wayne, bill, dax});

    assert_empty(
        employee_t::list()
            .where(employee_t::expr::hire_date <= date(1902, 5, 10)));
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
}

TEST_F(test_expressions, or_predicate)
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
}

TEST_F(test_expressions, and_expr)
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
}

TEST_F(test_expressions, not_expr)
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
}

TEST_F(test_expressions, mix_boolean_expr)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(
                (employee_t::expr::name_first == "Wayne" && employee_t::expr::name_last == "Warren")
                && employee_t::expr::hire_date < date(2036, 2, 7)),
        wayne);

    assert_contains(
        employee_t::list()
            .where(
                (employee_t::expr::name_first == "Wayne" && employee_t::expr::name_last == "Warren")
                || (employee_t::expr::hire_date > date(2036, 2, 7)
                    || employee_t::expr::hire_date == date(2019, 11, 15))),
        {wayne, dax});

    assert_empty(
        employee_t::list()
            .where(
                (employee_t::expr::name_first == "Wayne" && employee_t::expr::name_last == "Warren")
                && (employee_t::expr::hire_date > date(2036, 2, 7))));
}

TEST_F(test_expressions, container_contains_predicate)
{
    auto_transaction_t txn;
    //
    //    assert_contains(
    //        employee_t::list()
    //            .where(employee_t::expr::addressee_address_list
    //                       .contains(address_t::expr::state == "WA")),
    //        {dax, wayne, tobin, laurentiu, yiwen, mihir});
    //
    //    assert_empty(
    //        employee_t::list()
    //            .where(employee_t::expr::addressee_address_list
    //                       .contains(address_t::expr::state == "CA")));
}

TEST_F(test_expressions, container_contains_object)
{
    auto_transaction_t txn;

    assert_contains(
        employee_t::list()
            .where(employee_t::expr::addressee_address_list
                       .contains(bellevue)),
        {laurentiu});

    auto marzabotto = create_address("Marzabotto", "IT");

    assert_empty(
        employee_t::list()
            .where(employee_t::expr::addressee_address_list
                       .contains(marzabotto)));
}

template <typename T_class, typename = void>
struct is_container_t : std::false_type
{
};

template <typename... T_params>
struct is_container_helper_t
{
};

template <typename T_class>
struct is_container_t<
    T_class,
    std::conditional_t<
        false,
        is_container_helper_t<
            decltype(std::declval<T_class>().begin()),
            decltype(std::declval<T_class>().end())>,
        void>> : public std::true_type
{
};

int main()
{

    begin_session();
    begin_transaction();

    auto e = employee_t();
    auto addr = address_t();
    auto a = address_t::expr::state == "CA";
    cout << typeid(decltype(a)).name() << endl;
    cout << typeid(decltype(e)).name() << endl;
    cout << "std::is_invocable_v<T_predicate> " << std::is_invocable_v<decltype(a)> << endl;
    cout << "<std::is_base_of_v<edc_base_t, T_value> " << std::is_base_of_v<edc_base_t, decltype(e)> << endl;

    auto b = std::function<bool()>([]() { return true; });
    cout << typeid(decltype(b)).name() << endl;
    cout << "std::is_invocable_v<T_predicate> " << std::is_invocable_v<decltype(b)> << endl;

    auto c = []() { return true; };
    cout << typeid(decltype(c)).name() << endl;
    cout << "std::is_invocable_v<T_predicate> " << std::is_invocable_v<decltype(c)> << endl;

    cout << "Is container " << is_container_t<decltype(addr.phone_list())>::value << endl;
    cout << "Is container " << is_container_t<decltype(&address_t::state)>::value << endl;

    commit_transaction();
    end_session();
}
