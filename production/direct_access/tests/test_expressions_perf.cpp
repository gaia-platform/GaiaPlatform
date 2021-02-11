/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include <iostream>
#include <string>

#include <gtest/gtest.h>

#include "gaia/common.hpp"
#include "gaia/direct_access/auto_transaction.hpp"
#include "gaia/logger.hpp"

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_addr_book.h"

using namespace gaia::addr_book;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace std;

using g_timer_t = gaia::common::timer_t;

static const uint64_t c_num_employees = 100000;
static const uint64_t c_num_employee_addresses = 3;

/**
 * Test performance of the predicates built with the expression API vs
 * the performance of the same operation performed manually in a for-loop.
 *
 * NOTE: I've noticed a some variation in test results. This test suite is probably
 * weak. I'm not an expert in cpp micro-benchmarking. Long story short, the expression
 * API does not seem to be significantly slower than the normal for-loop, instead it
 * is usually faster by few percentage points (on my laptop).
 *
 * This is an example result with:
 *  c_num_employees=100k
 *  c_num_employee_addresses=3
 *
 * Comparing 'int64_t ==' performance:
 *  [expr]: 72512.69 us
 *  [plain]: 75772.47 us
 *  ->expr is 4.30% faster
 *
 * Comparing 'int64_t >' performance:
 *  [expr]: 78785.68 us
 *  [plain]: 76920.81 us
 *  ->expr is 2.42% slower
 *
 * Comparing 'int64_t <' performance:
 *  [expr]: 77221.09 us
 *  [plain]: 79437.41 us
 *  ->expr is 2.79% faster
 *
 * Comparing 'const char* ==' performance:
 *  [expr]: 113421.37 us
 *  [plain]: 120552.75 us
 *  ->expr is 5.92% faster
 *
 * Comparing 'std::string ==' performance:
 *  [expr]: 99087.56 us
 *  [plain]: 102789.92 us
 *  ->expr is 3.60% faster
 *
 * Comparing 'std::string equals ignore case' performance:
 *  [expr]: 87723.36 us
 *  [plain]: 84993.72 us
 *  ->expr is 3.21% slower
 *
 * Comparing 'std::string equals ignore case' performance:
 *  [expr]: 155836.77 us
 *  [plain]: 158854.93 us
 *  ->expr is 1.90% faster
 */
class test_expressions_perf : public db_catalog_test_base_t
{
public:
    test_expressions_perf()
        : db_catalog_test_base_t("addr_book.ddl"){};

    void SetUp() override
    {
        db_catalog_test_base_t::SetUp();

        auto_transaction_t txn;

        auto start = g_timer_t::get_time_point();

        for (uint64_t i = 0; i < c_num_employees; i++)
        {
            auto employee_writer = gaia::addr_book::employee_writer();
            employee_writer.name_first = "Name_" + to_string(i);
            employee_writer.name_last = "Surname_" + to_string(i);
            employee_writer.hire_date = static_cast<int64_t>(i);
            auto employee = employee_t::get(employee_writer.insert_row());

            for (uint64_t j = 0; j < c_num_employee_addresses; j++)
            {
                auto address_w = address_writer();
                address_w.city = "city_" + to_string(j);
                address_w.state = "state_" + to_string(j);
                employee.addressee_address_list().insert(address_w.insert_row());
            }

            if (i % 10000 == 0)
            {
                txn.commit();
                auto elapsed = g_timer_t::ns_to_ms(g_timer_t::get_duration(start));
                gaia_log::app().debug("Iteration:{} Time in ms:{}", i, elapsed);
                start = g_timer_t::get_time_point();
            }
        }

        txn.commit();
    }
};

double_t percentage_difference(int64_t expr, int64_t plain)
{
    return static_cast<double_t>(expr - plain) / plain * 100.0;
}

void log_performance_difference(int64_t expr_duration_ns, int64_t plain_duration_ns, std::string_view message)
{
    cout << "Comparing '" << message << "' performance:" << endl;
    printf(" [expr]: %0.2f us\n", g_timer_t::ns_to_us(expr_duration_ns));
    printf(" [plain]: %0.2f us\n", g_timer_t::ns_to_us(plain_duration_ns));

    double_t percentage_diff = percentage_difference(expr_duration_ns, plain_duration_ns);

    if (percentage_diff > 0)
    {
        printf(" ->expr is %0.2f%% slower\n", percentage_diff);
    }
    else
    {
        printf(" ->expr is %0.2f%% faster\n", abs(percentage_diff));
    }

    cout << endl;
}

TEST_F(test_expressions_perf, int_eq)
{
    auto_transaction_t txn;

    static const int64_t c_time = 100;

    int64_t expr_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            for (auto& e : employee_t::list()
                               .where(employee_t::expr::hire_date == c_time))
            {
                employees.push_back(e);
            }
            ASSERT_EQ(employees.size(), 1);
        });

    int64_t plain_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            for (auto& e : employee_t::list())
            {
                if (e.hire_date() == c_time)
                {
                    employees.push_back(e);
                }
            }
            ASSERT_EQ(employees.size(), 1);
        });

    log_performance_difference(expr_duration, plain_duration, "int64_t ==");

    txn.commit();
}

TEST_F(test_expressions_perf, int_gteq)
{
    auto_transaction_t txn;

    static const int64_t c_time = 10000;

    int64_t expr_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            for (auto& e : employee_t::list()
                               .where(employee_t::expr::hire_date >= c_time))
            {
                employees.push_back(e);
            }
            ASSERT_EQ(employees.size(), c_num_employees - c_time);
        });

    int64_t plain_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            for (auto& e : employee_t::list())
            {
                if (e.hire_date() >= c_time)
                {
                    employees.push_back(e);
                }
            }
            ASSERT_EQ(employees.size(), c_num_employees - c_time);
        });

    log_performance_difference(expr_duration, plain_duration, "int64_t >");

    txn.commit();
}

TEST_F(test_expressions_perf, int_lt)
{
    auto_transaction_t txn;

    static const int64_t c_time = 1000;

    int64_t expr_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            for (auto& e : employee_t::list()
                               .where(employee_t::expr::hire_date < c_time))
            {
                employees.push_back(e);
            }
            ASSERT_EQ(employees.size(), c_time);
        });

    int64_t plain_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            for (auto& e : employee_t::list())
            {
                if (e.hire_date() < c_time)
                {
                    employees.push_back(e);
                }
            }
            ASSERT_EQ(employees.size(), c_time);
        });

    log_performance_difference(expr_duration, plain_duration, "int64_t <");

    txn.commit();
}

TEST_F(test_expressions_perf, c_string_eq)
{
    auto_transaction_t txn;

    int64_t expr_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            for (auto& e : employee_t::list()
                               .where(employee_t::expr::name_first == "Name_1000"))
            {
                employees.push_back(e);
            }
            ASSERT_EQ(employees.size(), 1);
        });

    int64_t plain_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            const char* value = "Name_1000";
            for (auto& e : employee_t::list())
            {
                if (strcmp(e.name_first(), value) == 0)
                {
                    employees.push_back(e);
                }
            }
            ASSERT_EQ(employees.size(), 1);
        });

    log_performance_difference(expr_duration, plain_duration, "const char* ==");

    txn.commit();
}

TEST_F(test_expressions_perf, string_eq)
{
    auto_transaction_t txn;

    int64_t expr_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            for (auto& e : employee_t::list()
                               .where(employee_t::expr::name_first == string("Name_1000")))
            {
                employees.push_back(e);
            }
            ASSERT_EQ(employees.size(), 1);
        });

    int64_t plain_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            string value = "Name_1000";
            for (auto& e : employee_t::list())
            {
                if (strcmp(e.name_first(), value.c_str()) == 0)
                {
                    employees.push_back(e);
                }
            }
            ASSERT_EQ(employees.size(), 1);
        });

    log_performance_difference(expr_duration, plain_duration, "std::string ==");

    txn.commit();
}

TEST_F(test_expressions_perf, string_eq_case_insensitive)
{
    auto_transaction_t txn;

    int64_t expr_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            for (auto& e : employee_t::list()
                               .where(employee_t::expr::name_first
                                          .equals(string("name_1000"), string_comparison_t::case_insensitive)))
            {
                employees.push_back(e);
            }
            ASSERT_EQ(employees.size(), 1);
        });

    int64_t plain_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            string value = "name_1000";
            for (auto& e : employee_t::list())
            {
                string a = e.name_first();
                if (std::equal(
                        a.begin(), a.end(), value.begin(), value.end(),
                        [](const char& a, const char& b) {
                            return tolower(a) == tolower(b);
                        }))
                {
                    employees.push_back(e);
                }
            }
            ASSERT_EQ(employees.size(), 1);
        });

    log_performance_difference(expr_duration, plain_duration, "std::string equals ignore case");

    txn.commit();
}

TEST_F(test_expressions_perf, object_eq)
{
    auto_transaction_t txn;

    const employee_t dude = *employee_t::list()
                                 .where(employee_t::expr::name_first == "Name_0")
                                 .begin();
    ASSERT_TRUE(dude);

    int64_t expr_duration = g_timer_t::get_function_duration(
        [&dude]() {
            vector<address_t> addresses;
            for (auto& a : address_t::list()
                               .where(address_t::expr::addressee_employee == dude))
            {
                addresses.push_back(a);
            }
            ASSERT_EQ(addresses.size(), c_num_employee_addresses);
        });

    int64_t plain_duration = g_timer_t::get_function_duration(
        [&dude]() {
            vector<address_t> addresses;
            for (auto& a : address_t::list())
            {
                if (a.addressee_employee() == dude)
                {
                    addresses.push_back(a);
                }
            }
            ASSERT_EQ(addresses.size(), c_num_employee_addresses);
        });

    log_performance_difference(expr_duration, plain_duration, "std::string equals ignore case");

    txn.commit();
}
