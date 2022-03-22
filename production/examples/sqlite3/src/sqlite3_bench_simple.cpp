/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <sqlite3.h>

#include <cmath>
#include <cstdio>

#include <chrono>
#include <functional>
#include <iostream>
#include <limits>

#include "spdlog/spdlog.h"
#include <gtest/gtest.h>

#include "timer.hpp"

const constexpr uint32_t c_insert_buffer_stmts = 65000;
const constexpr uint32_t c_num_insertion = 10000000;

using namespace std::chrono;
using namespace std;

using g_timer_t = gaia::common::timer_t;

void exec(sqlite3* db, const char* query)
{
    char* err_msg = nullptr;

    int rc = sqlite3_exec(db, query, nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        exit(1);
    }
}

void assert_result(sqlite3* db, int rc)
{
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }
}

sqlite3_stmt* query(sqlite3* db, const char* query)
{

    sqlite3_stmt* res;
    int rc = sqlite3_prepare_v2(db, query, -1, &res, nullptr);

    assert_result(db, rc);

    return res;
}

void clear_database(sqlite3* db)
{

    int rc = sqlite3_open("file:cachedb?mode=memory&cache=shared", &db);
    assert_result(db, rc);

    exec(db, "DELETE FROM  simple_table");
}

template <typename T_num>
class accumulator_t
{
public:
    void add(T_num value)
    {
        if (value < m_min)
        {
            m_min = value;
        }

        if (value > m_max)
        {
            m_max = value;
        }

        m_sum += value;
        m_num_observations++;
    }

    T_num min()
    {
        return m_min;
    }

    T_num max()
    {
        return m_max;
    }

    double_t avg()
    {
        return m_sum / (m_num_observations + 0.0);
    }

private:
    T_num m_min = std::numeric_limits<T_num>::max();
    T_num m_max = std::numeric_limits<T_num>::min();
    T_num m_sum = 0;
    uint64_t m_num_observations = 0;
};

class sqlite3_benchmark : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        spdlog::set_level(spdlog::level::info);
    }

    static void TearDownTestCase()
    {
        spdlog::shutdown();
    }

    void SetUp() override
    {
        if (db != nullptr)
        {
            return;
        }

        int rc = sqlite3_open("file:cachedb?mode=memory&cache=shared", &db);

        assert_result(db, rc);

        printf("Autocommit: %d\n", sqlite3_get_autocommit(db));

        sqlite3_stmt* version_stmt = query(db, "SELECT SQLITE_VERSION()");

        sqlite3_step(version_stmt);
        printf("%s\n", sqlite3_column_text(version_stmt, 0));
        sqlite3_finalize(version_stmt);

        exec(db, "CREATE TABLE IF NOT EXISTS simple_table(uint64_field BIGINT);\n"
                 "PRAGMA user_version = 4;");
    }

    void log_performance_difference(accumulator_t<int64_t> expr_accumulator, std::string_view message, uint64_t num_insertions)
    {
        double_t avg_expr = g_timer_t::ns_to_us(expr_accumulator.avg());
        double_t avg_expr_ms = g_timer_t::ns_to_ms(expr_accumulator.avg());
        double_t min_expr = g_timer_t::ns_to_us(expr_accumulator.min());
        double_t max_expr = g_timer_t::ns_to_us(expr_accumulator.max());
        double_t single_record_insertion = avg_expr / num_insertions;

        cout << message << " performance:" << endl;
        printf(
            "  [expr]: avg:%0.2fus/%0.2fms min:%0.2fus max:%0.2fus single_insert:%0.2fus\n",
            avg_expr, avg_expr_ms, min_expr, max_expr, single_record_insertion);

        cout << endl;
    }

    void run_performance_test(
        std::function<void()> expr_fn,
        std::string_view message,
        size_t num_iterations = 3,
        uint64_t num_insertions = c_num_insertion)
    {
        accumulator_t<int64_t> expr_accumulator;

        for (size_t iteration = 0; iteration < num_iterations; iteration++)
        {
            spdlog::debug("[{}]: {} iteration staring, {} insertions", message, iteration, num_insertions);
            int64_t expr_duration = g_timer_t::get_function_duration(expr_fn);
            expr_accumulator.add(expr_duration);

            double_t iteration_ms = g_timer_t::ns_to_ms(expr_duration);
            spdlog::debug("[{}]: {} iteration, completed in {:.2f}ms", message, iteration, iteration_ms);

            spdlog::debug("[{}]: {} iteration, clearing database", message, iteration);
            int64_t clear_database_duration = g_timer_t::get_function_duration([this]() { clear_database(db); });
            double_t clear_ms = g_timer_t::ns_to_ms(clear_database_duration);
            spdlog::debug("[{}]: {} iteration, cleared in {:.2f}ms", message, iteration, clear_ms);
        }

        log_performance_difference(expr_accumulator, message, num_insertions);
    }

public:
    sqlite3* db = nullptr;
};

TEST_F(sqlite3_benchmark, simple_insert_string_multiple)
{
    auto simple_insert = [this]() {
        std::stringstream insert_ss;
        insert_ss << "BEGIN TRANSACTION;" << std::endl;
        insert_ss << "INSERT INTO simple_table(uint64_field) VALUES" << std::endl;

        for (int i = 0; i < c_num_insertion; i++)
        {
            insert_ss << "(" << i << ")" << std::endl;

            if (i % c_insert_buffer_stmts == 0 || i == c_num_insertion - 1)
            {
                insert_ss << ";COMMIT;";
                char* err_msg = nullptr;
                int rc = sqlite3_exec(db, insert_ss.str().c_str(), nullptr, nullptr, &err_msg);
                insert_ss = {};
                insert_ss << "BEGIN TRANSACTION;" << std::endl;
                insert_ss << "INSERT INTO simple_table(uint64_field) VALUES" << std::endl;

                if (rc != SQLITE_OK)
                {

                    fprintf(stderr, "SQL error: %s\n", err_msg);

                    sqlite3_free(err_msg);
                    sqlite3_close(db);

                    exit(1);
                }
            }
            else
            {
                insert_ss << ",";
            }
        }
    };

    run_performance_test(simple_insert, "sqlite3::simple_insert_string_multiple");
}

TEST_F(sqlite3_benchmark, simple_insert_string_single)
{
    auto simple_insert = [this]() {
        std::stringstream insert_ss;
        insert_ss << "BEGIN TRANSACTION;";

        for (int i = 0; i < c_num_insertion; i++)
        {
            insert_ss << "INSERT INTO simple_table(uint64_field) VALUES (" << i << ");" << std::endl;

            if (i % c_insert_buffer_stmts == 0 || i == c_num_insertion - 1)
            {
                insert_ss << "COMMIT;";
                char* err_msg = nullptr;
                int rc = sqlite3_exec(db, insert_ss.str().c_str(), nullptr, nullptr, &err_msg);
                insert_ss = {};
                insert_ss << "BEGIN TRANSACTION;";

                if (rc != SQLITE_OK)
                {

                    fprintf(stderr, "SQL error: %s\n", err_msg);

                    sqlite3_free(err_msg);
                    sqlite3_close(db);

                    exit(1);
                }
            }
        }
    };

    run_performance_test(simple_insert, "sqlite3::simple_insert_string_single");
}

TEST_F(sqlite3_benchmark, simple_insert_prepared_statement)
{
    auto simple_insert = [this]() {
        const char* sql = "INSERT INTO simple_table(uint64_field) VALUES (?);";

        sqlite3_stmt* stmt; // will point to prepared stamement object
        sqlite3_prepare_v2(
            db, // the handle to your (opened and ready) database
            sql, // the sql statement, utf-8 encoded
            -1, // max length of sql statement
            &stmt, // this is an "out" parameter, the compiled statement goes here
            nullptr); // pointer to the tail end of sql statement (when there are
                      // multiple statements inside the string; can be null)

        exec(db, "BEGIN TRANSACTION;");

        for (int i = 0; i < c_num_insertion; i++)
        {
            sqlite3_bind_int64(stmt, 1, i);
            int ret_val = sqlite3_step(stmt);
            if (ret_val != SQLITE_DONE)
            {
                printf("Commit Failed! %d\n", ret_val);
                sqlite3_close(db);
                exit(1);
            }
            sqlite3_reset(stmt);

            if (i % c_insert_buffer_stmts == 0 || i == c_num_insertion - 1)
            {
                exec(db, "COMMIT;");

                if (i < c_num_insertion - 1)
                {
                    exec(db, "BEGIN TRANSACTION;");
                }
            }
        }
    };

    run_performance_test(simple_insert, "sqlite3::simple_insert_prepared_statement");
}

// int main()
//{
//
//     sqlite3* db;
//
//     int rc = sqlite3_open(":memory:", &db);
//
//     assert_result(db, rc);
//
//     printf("Autocommit: %d\n", sqlite3_get_autocommit(db));
//
//     sqlite3_stmt* version_stmt = query(db, "SELECT SQLITE_VERSION()");
//
//     sqlite3_step(version_stmt);
//     printf("%s\n", sqlite3_column_text(version_stmt, 0));
//     sqlite3_finalize(version_stmt);
//
//     exec(db, "CREATE TABLE IF NOT EXISTS simple_table(uint64_field BIGINT);\n"
//              "PRAGMA user_version = 4;");
//
//     sqlite3_stmt* count_stmt = query(db, "SELECT COUNT(*) FROM Vertices");
//
//     sqlite3_step(count_stmt);
//     int count = sqlite3_column_int(count_stmt, 0);
//     sqlite3_finalize(count_stmt);
//
//     if (count == 0)
//     {
//         printf("No vertexes, generating data...\n");
//         generate_data(db);
//     }
//     else
//     {
//         printf("Num vertexes: %d\n", count);
//     }
//
//     query_data(db);
//
//     sqlite3_close(db);
//
//     return 0;
// }

// void query_data(sqlite3* db)
//{
//     steady_clock::time_point begin = std::chrono::steady_clock::now();
//     sqlite3_stmt* stmt = query(db, "SELECT id FROM Vertices WHERE type = ?");
//     sqlite3_bind_int(stmt, 1, 1);
//
//     int i = 0;
//
//     while (sqlite3_step(stmt) != SQLITE_DONE)
//     {
//         sqlite3_column_int64(stmt, 0);
//         i++;
//     }
//     steady_clock::time_point end = std::chrono::steady_clock::now();
//
//     auto elapsed = std::chrono::duration_cast<milliseconds>(end - begin).count();
//
//     printf("Num of vertexes of type 1 %d in %ldms\n", i, elapsed);
//     sqlite3_finalize(stmt);
// }
