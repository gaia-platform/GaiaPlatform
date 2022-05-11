/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_addr_book.h"
#include "test_perf.hpp"

using namespace gaia::addr_book;
using namespace gaia::addr_book::address_expr;
using namespace gaia::addr_book::employee_expr;
using namespace gaia::benchmark;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace std;

using g_timer_t = gaia::common::timer_t;

// For the tests to succeed:
// - c_num_employees must be >= 101
// - c_num_employee_addresses must be >= 1
static const int64_t c_num_employees = 1000;
static const int64_t c_num_employee_addresses = 2;

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
 *  c_num_employee_addresses=2
 *  iteration=10
 *
 * Comparing 'int64_t ==' performance:
 *   [expr]: avg:71202.56us min:68605.46us max:76158.40us
 *  [plain]: avg:73750.23us min:69868.34us max:78082.55us
 *  ->expr is 3.45% faster
 *
 * Comparing 'int64_t >' performance:
 *   [expr]: avg:71434.88us min:66800.11us max:75009.76us
 *  [plain]: avg:72051.07us min:66190.02us max:76080.16us
 *  ->expr is 0.86% faster
 *
 * Comparing 'int64_t <' performance:
 *   [expr]: avg:68504.45us min:62284.45us max:75248.46us
 *  [plain]: avg:71111.32us min:66372.92us max:74158.02us
 *  ->expr is 3.67% faster
 *
 * Comparing 'const char* ==' performance:
 *   [expr]: avg:80654.28us min:75795.57us max:86061.24us
 *  [plain]: avg:84660.81us min:80487.47us max:88559.94us
 *  ->expr is 4.73% faster
 *
 * Comparing 'std::string ==' performance:
 *   [expr]: avg:81805.88us min:76163.18us max:89563.08us
 *  [plain]: avg:86178.28us min:80889.47us max:91281.46us
 *  ->expr is 5.07% faster
 *
 * Comparing 'DAC class ==' performance:
 *   [expr]: avg:126928.02us min:124232.26us max:133843.79us
 *  [plain]: avg:131983.32us min:129818.73us max:136030.87us
 *  ->expr is 3.83% faster
 *
 * Comparing 'mixed boolean op' performance:
 *   [expr]: avg:110666.83us min:104651.21us max:117001.52us
 *  [plain]: avg:113004.57us min:106622.64us max:120016.23us
 *  ->expr is 2.07% faster
 *
 * Comparing 'container contains' performance:
 *   [expr]: avg:215022.69us min:209438.60us max:225074.10us
 *  [plain]: avg:206749.38us min:202125.78us max:211504.21us
 *  ->expr is 4.00% slower
 *
 * Comparing 'container contains lambda' performance:
 *   [expr]: avg:213658.99us min:209991.27us max:226889.45us
 *  [plain]: avg:186504.27us min:182121.90us max:190658.59us
 *  ->expr is 14.56% slower
 *
 * Comparing 'container count' performance:
 *   [expr]: avg:161366.45us min:155702.49us max:177688.25us
 *  [plain]: avg:159336.31us min:153831.77us max:175199.49us
 *  ->expr is 1.27% slower
 *
 * Comparing 'container empty' performance:
 *   [expr]: avg:116276.95us min:109341.40us max:126678.42us
 *  [plain]: avg:95348.92us min:87834.60us max:107550.61us
 *  ->expr is 21.95% slower
 *  */
class direct_access__perf_expressions : public gaia::db::db_catalog_test_base_t
{
public:
    direct_access__perf_expressions()
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
                employee.addresses().insert(address_w.insert_row());
            }

            // Ensuring we don't hit the transaction limit.
            if (index_employee % 1000 == 0)
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

void log_performance_difference(
    accumulator_t<int64_t> expr_accumulator,
    accumulator_t<int64_t> plain_accumulator,
    std::string_view message)
{
    double_t avg_expr = g_timer_t::ns_to_us(expr_accumulator.avg());
    double_t min_expr = g_timer_t::ns_to_us(expr_accumulator.min());
    double_t max_expr = g_timer_t::ns_to_us(expr_accumulator.max());
    double_t avg_plain = g_timer_t::ns_to_us(plain_accumulator.avg());
    double_t min_plain = g_timer_t::ns_to_us(plain_accumulator.min());
    double_t max_plain = g_timer_t::ns_to_us(plain_accumulator.max());

    cout << "Comparing '" << message << "' performance:" << endl;
    printf("  [expr]: avg:%0.2fus min:%0.2fus max:%0.2fus\n", avg_expr, min_expr, max_expr);
    printf(" [plain]: avg:%0.2fus min:%0.2fus max:%0.2fus\n", avg_plain, min_plain, max_plain);

    double_t percentage_diff = percentage_difference(avg_expr, avg_plain);

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

void run_expr_performance_test(
    std::function<void()> expr_fn,
    std::function<void()> plain_fn,
    std::string_view message,
    uint32_t num_iterations = 5)
{
    accumulator_t<int64_t> expr_accumulator;
    accumulator_t<int64_t> plain_accumulator;

    for (uint32_t iteration = 0; iteration < num_iterations; iteration++)
    {
        int64_t expr_duration = g_timer_t::get_function_duration(expr_fn);
        expr_accumulator.add(expr_duration);

        int64_t plain_duration = g_timer_t::get_function_duration(plain_fn);
        plain_accumulator.add(plain_duration);
    }

    log_performance_difference(expr_accumulator, plain_accumulator, message);
}

TEST_F(direct_access__perf_expressions, int_eq)
{
    auto_transaction_t txn;

    static const int64_t c_time = 100;

    auto expr_fn = []() {
        auto container = employee_t::list()
                             .where(hire_date == c_time);
        int num_employee = std::distance(container.begin(), container.end());

        EXPECT_EQ(num_employee, 1);
    };

    auto plain_fn = []() {
        int num_employee = 0;
        for (auto& e : employee_t::list())
        {
            if (e.hire_date() == c_time)
            {
                num_employee++;
            }
        }
        EXPECT_EQ(num_employee, 1);
    };

    run_expr_performance_test(expr_fn, plain_fn, "int64_t ==");

    txn.commit();
}

TEST_F(direct_access__perf_expressions, int_gteq)
{
    auto_transaction_t txn;

    static const int64_t c_time = 100;

    auto expr_fn = []() {
        auto container = employee_t::list()
                             .where(hire_date >= c_time);
        int num_employee = std::distance(container.begin(), container.end());

        EXPECT_EQ(num_employee, c_num_employees - c_time);
    };

    auto plain_fn = []() {
        int num_employee = 0;
        for (auto& e : employee_t::list())
        {
            if (e.hire_date() >= c_time)
            {
                num_employee++;
            }
        }
        EXPECT_EQ(num_employee, c_num_employees - c_time);
    };

    run_expr_performance_test(expr_fn, plain_fn, "int64_t >");

    txn.commit();
}

TEST_F(direct_access__perf_expressions, int_lt)
{
    auto_transaction_t txn;

    static const int64_t c_time = 100;

    auto expr_fn = []() {
        auto container = employee_t::list()
                             .where(hire_date < c_time);
        int num_employee = std::distance(container.begin(), container.end());

        EXPECT_EQ(num_employee, c_time);
    };

    auto plain_fn = []() {
        int num_employee = 0;
        for (auto& e : employee_t::list())
        {
            if (e.hire_date() < c_time)
            {
                num_employee++;
            }
        }
        EXPECT_EQ(num_employee, c_time);
    };

    run_expr_performance_test(expr_fn, plain_fn, "int64_t <");

    txn.commit();
}

TEST_F(direct_access__perf_expressions, c_string_eq)
{
    auto_transaction_t txn;

    auto expr_fn = []() {
        auto container = employee_t::list()
                             .where(name_first == "Name_100");
        int num_employee = std::distance(container.begin(), container.end());

        EXPECT_EQ(num_employee, 1);
    };

    auto plain_fn = []() {
        int num_employee = 0;
        const char* value = "Name_100";
        for (auto& e : employee_t::list())
        {
            if (strcmp(e.name_first(), value) == 0)
            {
                num_employee++;
            }
        }
        EXPECT_EQ(num_employee, 1);
    };

    run_expr_performance_test(expr_fn, plain_fn, "const char* ==");

    txn.commit();
}

TEST_F(direct_access__perf_expressions, string_eq)
{
    auto_transaction_t txn;

    auto expr_fn = []() {
        auto container = employee_t::list()
                             .where(name_first == string("Name_100"));
        int num_employee = std::distance(container.begin(), container.end());

        EXPECT_EQ(num_employee, 1);
    };

    auto plain_fn = []() {
        int num_employee = 0;
        string value = "Name_100";
        for (auto& e : employee_t::list())
        {
            if (strcmp(e.name_first(), value.c_str()) == 0)
            {
                num_employee++;
            }
        }
        EXPECT_EQ(num_employee, 1);
    };

    run_expr_performance_test(expr_fn, plain_fn, "std::string ==");

    txn.commit();
}

TEST_F(direct_access__perf_expressions, object_eq)
{
    auto_transaction_t txn;

    const employee_t dude = *employee_t::list()
                                 .where(name_first == "Name_0")
                                 .begin();
    EXPECT_TRUE(dude);

    auto expr_fn = [&dude]() {
        vector<address_t> addresses;
        for (auto& a : address_t::list()
                           .where(owner == dude))
        {
            addresses.push_back(a);
        }
        EXPECT_EQ(addresses.size(), c_num_employee_addresses);
    };

    auto plain_fn = [&dude]() {
        vector<address_t> addresses;
        for (auto& a : address_t::list())
        {
            if (a.owner() == dude)
            {
                addresses.push_back(a);
            }
        }
        EXPECT_EQ(addresses.size(), c_num_employee_addresses);
    };

    run_expr_performance_test(expr_fn, plain_fn, "DAC class ==");

    txn.commit();
}

TEST_F(direct_access__perf_expressions, mixed_bool_op)
{
    auto_transaction_t txn;

    auto expr_fn = []() {
        auto container = employee_t::list()
                             .where(
                                 (name_first == "Name_100" && name_last == "Surname_100")
                                 || (hire_date >= 10 && hire_date < 11));
        int num_employee = std::distance(container.begin(), container.end());

        EXPECT_EQ(num_employee, 2);
    };

    auto plain_fn = []() {
        int num_employee = 0;
        for (auto& e : employee_t::list())
        {
            if ((strcmp(e.name_first(), "Name_100") == 0 && strcmp(e.name_last(), "Surname_100") == 0)
                || (e.hire_date() >= 10 && e.hire_date() < 11))
            {
                num_employee++;
            }
        }
        EXPECT_EQ(num_employee, 2);
    };

    run_expr_performance_test(expr_fn, plain_fn, "mixed boolean op");

    txn.commit();
}

// TODO: Test broken: https://gaiaplatform.atlassian.net/browse/GAIAPLAT-2059
TEST_F(direct_access__perf_expressions, container_contains)
{
    auto_transaction_t txn;

    auto expr_fn = []() {
        auto container = employee_t::list()
                             .where(addresses.contains(city == "city_0"));
        int num_employee = std::distance(container.begin(), container.end());

        EXPECT_EQ(num_employee, c_num_employees);
    };

    auto plain_fn = []() {
        int num_employee = 0;
        for (auto& e : employee_t::list())
        {
            for (auto& a : e.addresses())
            {
                if (string("city_0") == a.city())
                {
                    num_employee++;
                }
            }
        }
        EXPECT_EQ(num_employee, c_num_employees);
    };

    run_expr_performance_test(expr_fn, plain_fn, "container contains");

    txn.commit();
}

// TODO: Test broken: https://gaiaplatform.atlassian.net/browse/GAIAPLAT-2059
TEST_F(direct_access__perf_expressions, container_contains_lambda)
{
    auto_transaction_t txn;

    auto expr_fn = []() {
        auto container = employee_t::list()
                             .where(addresses.contains(city == "city_0"));
        int num_employee = std::distance(container.begin(), container.end());

        EXPECT_EQ(num_employee, c_num_employees);
    };

    auto plain_fn = []() {
        auto container = employee_t::list().where(
            [](const employee_t& emp) {
                auto addresses = emp.addresses().where(
                    [](const address_t addr) {
                        return addr.city() == string("city_0");
                    });
                return addresses.begin() != addresses.end();
            });
        int num_employee = std::distance(container.begin(), container.end());

        EXPECT_EQ(num_employee, c_num_employees);
    };

    run_expr_performance_test(expr_fn, plain_fn, "container contains lambda");

    txn.commit();
}

// TODO: Test broken: https://gaiaplatform.atlassian.net/browse/GAIAPLAT-2059
TEST_F(direct_access__perf_expressions, container_count)
{
    auto_transaction_t txn;

    auto expr_fn = []() {
        auto container = employee_t::list()
                             .where(addresses.count() == c_num_employee_addresses);
        int num_employee = std::distance(container.begin(), container.end());

        EXPECT_EQ(num_employee, c_num_employees);
    };

    auto plain_fn = []() {
        int num_employee = 0;
        for (auto& e : employee_t::list())
        {
            auto addresses = e.addresses();
            if (std::distance(addresses.begin(), addresses.end()) == c_num_employee_addresses)
            {
                num_employee++;
            }
        }
        EXPECT_EQ(num_employee, c_num_employees);
    };

    run_expr_performance_test(expr_fn, plain_fn, "container count");

    txn.commit();
}

// TODO: Test broken: https://gaiaplatform.atlassian.net/browse/GAIAPLAT-2059
TEST_F(direct_access__perf_expressions, container_empty)
{
    auto_transaction_t txn;

    auto expr_fn = []() {
        auto container = employee_t::list()
                             .where(addresses.empty());
        int num_employee = std::distance(container.begin(), container.end());

        EXPECT_EQ(num_employee, 0);
    };

    auto plain_fn = []() {
        int num_employee = 0;
        for (auto& e : employee_t::list())
        {
            auto addresses = e.addresses();
            if (addresses.begin() == addresses.end())
            {
                num_employee++;
            }
        }
        EXPECT_EQ(num_employee, 0);
    };

    run_expr_performance_test(expr_fn, plain_fn, "container empty");

    txn.commit();
}
