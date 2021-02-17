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

#include "gaia_internal/common/timer.hpp"
#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_addr_book.h"

using namespace gaia::addr_book;
using namespace gaia::addr_book::address_expr;
using namespace gaia::addr_book::employee_expr;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace std;

using g_timer_t = gaia::common::timer_t;

static const uint64_t c_num_employees = 100000;
static const uint64_t c_num_employee_addresses = 3;

/**
 * Test the performance of the predicates built with the expression API vs
 * the performance of the same operation performed manually in a for-loop.
 *
 * NOTE: I've noticed some variation in test results. This test suite is probably
 * weak. I'm not an expert in cpp micro-benchmarking. Long story short, the expression
 * API does not seem to be significantly slower than the normal for-loop, instead it
 * is usually faster by a few percentage points (on my laptop).
 *
 * This is an example result with (built with -DCMAKE_BUILD_TYPE=Release):
 *  c_num_employees=100k
 *  c_num_employee_addresses=3
 *
 * Comparing 'int64_t ==' performance:
 *  [expr]: 75713.92 us
 *  [plain]: 79738.08 us
 *  ->expr is 5.05% faster
 *
 * Comparing 'int64_t >' performance:
 *  [expr]: 85465.62 us
 *  [plain]: 85384.19 us
 *  ->expr is 0.10% slower
 *
 * Comparing 'int64_t <' performance:
 *  [expr]: 80908.72 us
 *  [plain]: 78395.23 us
 *  ->expr is 3.21% slower
 *
 * Comparing 'const char* ==' performance:
 *  [expr]: 89520.14 us
 *  [plain]: 89476.31 us
 *  ->expr is 0.05% slower
 *
 * Comparing 'std::string ==' performance:
 *  [expr]: 84398.43 us
 *  [plain]: 94044.34 us
 *  ->expr is 10.26% faster
 *
 * Comparing 'std::string equals ignore case' performance:
 *  [expr]: 92790.74 us
 *  [plain]: 91392.17 us
 *  ->expr is 1.53% slower
 *
 * Comparing 'EDC class ==' performance:
 *  [expr]: 166484.30 us
 *  [plain]: 169587.11 us
 *  ->expr is 1.83% faster
 *
 * Comparing 'mixed boolean op' performance:
 *  [expr]: 116633.21 us
 *  [plain]: 118581.93 us
 *  ->expr is 1.64% faster
 *
 * Comparing 'container contains' performance:
 *  [expr]: 299694.76 us
 *  [plain]: 364134.03 us
 *  ->expr is 17.70% faster
 *
 * Comparing 'container count' performance:
 *  [expr]: 225746.42 us
 *  [plain]: 210859.86 us
 *  ->expr is 7.06% slower
 *
 * Comparing 'container empty' performance:
 *  [expr]: 130322.46 us
 *  [plain]: 114321.26 us
 *  ->expr is 14.00% slower
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

        for (uint64_t index_employee = 0; index_employee < c_num_employees; index_employee++)
        {
            auto employee_writer = gaia::addr_book::employee_writer();
            employee_writer.name_first = "Name_" + to_string(index_employee);
            employee_writer.name_last = "Surname_" + to_string(index_employee);
            employee_writer.hire_date = static_cast<int64_t>(index_employee);
            auto employee = employee_t::get(employee_writer.insert_row());

            for (uint64_t index_address = 0; index_address < c_num_employee_addresses; index_address++)
            {
                auto address_w = address_writer();
                address_w.city = "city_" + to_string(index_address);
                address_w.state = "state_" + to_string(index_address);
                employee.addressee_address_list().insert(address_w.insert_row());
            }

            if (index_employee % 10000 == 0)
            {
                txn.commit();
                auto elapsed = g_timer_t::ns_to_ms(g_timer_t::get_duration(start));
                gaia_log::app().debug("Iteration:{} Time in ms:{}", index_employee, elapsed);
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
                               .where(hire_date == c_time))
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
                               .where(hire_date >= c_time))
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
                               .where(hire_date < c_time))
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
                               .where(name_first == "Name_1000"))
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
                               .where(name_first == string("Name_1000")))
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

TEST_F(test_expressions_perf, object_eq)
{
    auto_transaction_t txn;

    const employee_t dude = *employee_t::list()
                                 .where(name_first == "Name_0")
                                 .begin();
    ASSERT_TRUE(dude);

    int64_t expr_duration = g_timer_t::get_function_duration(
        [&dude]() {
            vector<address_t> addresses;
            for (auto& a : address_t::list()
                               .where(addressee_employee == dude))
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

    log_performance_difference(expr_duration, plain_duration, "EDC class ==");

    txn.commit();
}

TEST_F(test_expressions_perf, mixed_bool_op)
{
    auto_transaction_t txn;

    int64_t expr_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            for (auto& e : employee_t::list()
                               .where(
                                   (name_first == "Name_1000" && name_last == "Surname_1000")
                                   || (hire_date >= 100 && hire_date < 101)))
            {
                employees.push_back(e);
            }
            ASSERT_EQ(employees.size(), 2);
        });

    int64_t plain_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            for (auto& e : employee_t::list())
            {
                if ((strcmp(e.name_first(), "Name_1000") == 0 && strcmp(e.name_last(), "Surname_1000") == 0)
                    || (e.hire_date() >= 100 && e.hire_date() < 101))
                {
                    employees.push_back(e);
                }
            }
            ASSERT_EQ(employees.size(), 2);
        });

    log_performance_difference(expr_duration, plain_duration, "mixed boolean op");

    txn.commit();
}

TEST_F(test_expressions_perf, test_container_contains)
{
    auto_transaction_t txn;

    int64_t expr_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            for (auto& e : employee_t::list()
                               .where(addressee_address_list.contains(city == "city_1")))
            {
                employees.push_back(e);
            }
            ASSERT_EQ(employees.size(), c_num_employees);
        });

    int64_t plain_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            for (auto& e : employee_t::list())
            {
                for (auto& a : e.addressee_address_list())
                {
                    if (string("city_1") == a.city())
                    {
                        employees.push_back(e);
                    }
                }
            }
            ASSERT_EQ(employees.size(), c_num_employees);
        });

    log_performance_difference(expr_duration, plain_duration, "container contains");

    txn.commit();
}

TEST_F(test_expressions_perf, test_container_count)
{
    auto_transaction_t txn;

    int64_t expr_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            for (auto& e : employee_t::list()
                               .where(addressee_address_list.count(c_num_employee_addresses)))
            {
                employees.push_back(e);
            }
            ASSERT_EQ(employees.size(), c_num_employees);
        });

    int64_t plain_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            for (auto& e : employee_t::list())
            {
                auto addresses = e.addressee_address_list();
                if (std::distance(addresses.begin(), addresses.end()) == c_num_employee_addresses)
                {
                    employees.push_back(e);
                }
            }
            ASSERT_EQ(employees.size(), c_num_employees);
        });

    log_performance_difference(expr_duration, plain_duration, "container count");

    txn.commit();
}

TEST_F(test_expressions_perf, test_container_empty)
{
    auto_transaction_t txn;

    int64_t expr_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            for (auto& e : employee_t::list()
                               .where(addressee_address_list.empty()))
            {
                employees.push_back(e);
            }
            ASSERT_TRUE(employees.empty());
        });

    int64_t plain_duration = g_timer_t::get_function_duration(
        []() {
            vector<employee_t> employees;
            for (auto& e : employee_t::list())
            {
                auto addresses = e.addressee_address_list();
                if (addresses.begin() == addresses.end())
                {
                    employees.push_back(e);
                }
            }
            ASSERT_TRUE(employees.empty());
        });

    log_performance_difference(expr_duration, plain_duration, "container empty");

    txn.commit();
}
