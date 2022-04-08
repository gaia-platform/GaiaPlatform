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

const constexpr uint32_t c_num_insertion = 10000000;
const constexpr uint32_t c_num_iterations = 3;
const constexpr uint32_t max_insertion_single_txn = c_num_insertion > 1000000 ? c_num_insertion / 10 : c_num_insertion;

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
    exec(db, "DELETE FROM  simple_table_3");
    exec(db, "DELETE FROM  unique_index_table");
    exec(db, "DELETE FROM  range_index_table");
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

        exec(db, "CREATE TABLE IF NOT EXISTS simple_table(\n"
                 "  uint64_field BIGINT\n"
                 ");\n"
                 "----------------------------------------\n"
                 "CREATE TABLE IF NOT EXISTS simple_table_3(\n"
                 "  uint64_field_1 BIGINT,\n"
                 "  uint64_field_2 BIGINT,\n"
                 "  uint64_field_3 BIGINT,\n"
                 "  uint64_field_4 BIGINT,\n"
                 "  string_field_1 TEXT,\n"
                 "  string_field_2 TEXT,\n"
                 "  string_field_3 TEXT,\n"
                 "  string_field_4 TEXT\n"
                 ");\n"
                 "----------------------------------------\n"
                 "CREATE TABLE IF NOT EXISTS unique_index_table(\n"
                 "  uint64_field BIGINT\n"
                 ");\n"
                 "CREATE UNIQUE INDEX unique_idx \n"
                 "ON unique_index_table(uint64_field);\n"
                 "----------------------------------------\n"
                 "CREATE TABLE IF NOT EXISTS range_index_table(\n"
                 "  uint64_field BIGINT\n"
                 ");\n"
                 "CREATE INDEX range_idx \n"
                 "ON range_index_table(uint64_field);\n"
                 "----------------------------------------\n");
    }

    void log_performance_difference(accumulator_t<int64_t> expr_accumulator, std::string_view message, uint64_t num_insertions, size_t num_iterations)
    {
        double_t avg_expr_us = g_timer_t::ns_to_us(static_cast<int64_t>(expr_accumulator.avg()));
        double_t min_expr_us = g_timer_t::ns_to_us(expr_accumulator.min());
        double_t max_expr_us = g_timer_t::ns_to_us(expr_accumulator.max());
        double_t avg_expr_ms = g_timer_t::ns_to_ms(static_cast<int64_t>(expr_accumulator.avg()));
        double_t min_expr_ms = g_timer_t::ns_to_ms(expr_accumulator.min());
        double_t max_expr_ms = g_timer_t::ns_to_ms(expr_accumulator.max());

        double_t single_record_avg_ns = avg_expr_us / static_cast<double_t>(num_insertions);
        double_t single_record_min_ns = min_expr_us / static_cast<double_t>(num_insertions);
        double_t single_record_max_ns = max_expr_us / static_cast<double_t>(num_insertions);

        printf(
            "[%s] %lu rows, %zu iterations:\n"
            "   [total]: avg:%0.2fms min:%0.2fms max:%0.2fms\n"
            "  [single]: avg:%0.2fus min:%0.2fus max:%0.2fus",
            message.data(), num_insertions, num_iterations,
            avg_expr_ms, min_expr_ms, max_expr_ms,
            single_record_avg_ns, single_record_min_ns, single_record_max_ns);

        cout << endl;
    }

    void run_performance_test(
        std::function<void()> expr_fn,
        std::string_view message,
        size_t num_iterations = c_num_iterations,
        uint64_t num_insertions = c_num_insertion,
        bool read_benchmark = false)
    {
        accumulator_t<int64_t> expr_accumulator;

        for (size_t iteration = 0; iteration < num_iterations; iteration++)
        {
            spdlog::info("[{}]: {} iteration staring, {} insertions", message, iteration, num_insertions);
            int64_t expr_duration = g_timer_t::get_function_duration(expr_fn);
            expr_accumulator.add(expr_duration);

            double_t iteration_ms = g_timer_t::ns_to_ms(expr_duration);
            spdlog::info("[{}]: {} iteration, completed in {:.2f}ms", message, iteration, iteration_ms);

            // Writer benchmarks need to reset the data on every iteration.
            if (!read_benchmark)
            {
                spdlog::debug("[{}]: {} iteration, clearing database", message, iteration);
                int64_t clear_database_duration = g_timer_t::get_function_duration([this]() { clear_database(db); });
                double_t clear_ms = g_timer_t::ns_to_ms(clear_database_duration);
                spdlog::debug("[{}]: {} iteration, cleared in {:.2f}ms", message, iteration, clear_ms);
            }
        }

        // Reader benchmarks need to use the data on every iteration hence the deletion should happen only at the end.
        if (read_benchmark)
        {
            spdlog::debug("[{}]: clearing database", message);
            int64_t clear_database_duration = g_timer_t::get_function_duration([this]() { clear_database(db); });
            double_t clear_ms = g_timer_t::ns_to_ms(clear_database_duration);
            spdlog::debug("[{}]: cleared in {:.2f}ms", message, clear_ms);
        }

        log_performance_difference(expr_accumulator, message, num_insertions, num_iterations);
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

            if (i % max_insertion_single_txn == 0 || i == c_num_insertion - 1)
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

            if (i % max_insertion_single_txn == 0 || i == c_num_insertion - 1)
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

            if (i % max_insertion_single_txn == 0 || i == c_num_insertion - 1)
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

TEST_F(sqlite3_benchmark, simple_insert_txn_size)
{
    for (int64_t txn_size : {1, 10, 100, 1000, 10000})
    {
        auto simple_insert = [this, txn_size]() {
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

                if (i % txn_size == 0 || i == c_num_insertion - 1)
                {
                    exec(db, "COMMIT;");

                    if (i < c_num_insertion - 1)
                    {
                        exec(db, "BEGIN TRANSACTION;");
                    }
                }
            }
        };

        run_performance_test(simple_insert, fmt::format("sqlite3::simple_insert with txn of size {}", txn_size), c_num_iterations, c_num_insertion);
    }
}

TEST_F(sqlite3_benchmark, simple_insert_3)
{
    auto simple_insert = [this]() {
        const char* sql = "INSERT INTO simple_table_3("
                          " uint64_field_1,"
                          " uint64_field_2,"
                          " uint64_field_3,"
                          " uint64_field_4,"
                          " string_field_1,"
                          " string_field_2,"
                          " string_field_3,"
                          " string_field_4) "
                          "VALUES (?,?,?,?,?,?,?,?);";

        sqlite3_stmt* stmt; // will point to prepared statement object
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
            sqlite3_bind_int64(stmt, 2, i);
            sqlite3_bind_int64(stmt, 3, i);
            sqlite3_bind_int64(stmt, 4, i);
            sqlite3_bind_text(stmt, 5, "aa", 2, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 6, "bb", 2, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 7, "cc", 2, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 8, "dd", 2, SQLITE_STATIC);

            int ret_val = sqlite3_step(stmt);
            if (ret_val != SQLITE_DONE)
            {
                printf("Commit Failed! %d\n", ret_val);
                sqlite3_close(db);
                exit(1);
            }
            sqlite3_reset(stmt);

            if (i % max_insertion_single_txn == 0 || i == c_num_insertion - 1)
            {
                exec(db, "COMMIT;");

                if (i < c_num_insertion - 1)
                {
                    exec(db, "BEGIN TRANSACTION;");
                }
            }
        }
    };

    run_performance_test(simple_insert, "sqlite3::simple_insert_3");
}

TEST_F(sqlite3_benchmark, unique_index_table)
{
    auto simple_insert = [this]() {
        const char* sql = "INSERT INTO unique_index_table(uint64_field) VALUES (?);";

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

            if (i % max_insertion_single_txn == 0 || i == c_num_insertion - 1)
            {
                exec(db, "COMMIT;");

                if (i < c_num_insertion - 1)
                {
                    exec(db, "BEGIN TRANSACTION;");
                }
            }
        }
    };

    run_performance_test(simple_insert, "sqlite3::unique_index_table");
}

TEST_F(sqlite3_benchmark, range_index_table)
{
    auto simple_insert = [this]() {
        const char* sql = "INSERT INTO range_index_table(uint64_field) VALUES (?);";

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

            if (i % max_insertion_single_txn == 0 || i == c_num_insertion - 1)
            {
                exec(db, "COMMIT;");

                if (i < c_num_insertion - 1)
                {
                    exec(db, "BEGIN TRANSACTION;");
                }
            }
        }
    };

    run_performance_test(simple_insert, "sqlite3::range_index_table");
}

void insert_records(sqlite3* db)
{
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

        if (i % max_insertion_single_txn == 0 || i == c_num_insertion - 1)
        {
            exec(db, "COMMIT;");

            if (i < c_num_insertion - 1)
            {
                exec(db, "BEGIN TRANSACTION;");
            }
        }
    }

    sqlite3_free(stmt);
}

TEST_F(sqlite3_benchmark, full_table_scan)
{
    insert_records(db);

    auto simple_insert = [this]() {
        sqlite3_stmt* stmt = query(db, "SELECT * FROM simple_table");

        int i = 0;

        while (sqlite3_step(stmt) != SQLITE_DONE)
        {
            i++;
        }

        sqlite3_finalize(stmt);

        ASSERT_EQ(c_num_insertion, i);
    };

    run_performance_test(simple_insert, "sqlite3::full_table_scan", c_num_iterations, c_num_insertion, true);
}

TEST_F(sqlite3_benchmark, full_table_scan_access_data)
{
    insert_records(db);

    auto simple_insert = [this]() {
        sqlite3_stmt* stmt = query(db, "SELECT * FROM simple_table");

        int i = 0;

        while (sqlite3_step(stmt) != SQLITE_DONE)
        {
            sqlite3_column_int64(stmt, 0);
            sqlite3_column_int64(stmt, 1);
            i++;
        }

        sqlite3_finalize(stmt);

        ASSERT_EQ(c_num_insertion, i);
    };

    run_performance_test(simple_insert, "sqlite3::full_table_scan_access_data", c_num_iterations, c_num_insertion, true);
}

TEST_F(sqlite3_benchmark, filter_no_match)
{
    insert_records(db);

    auto simple_insert = [this]() {
        sqlite3_stmt* stmt = query(db, "SELECT * FROM simple_table WHERE uint64_field > ?");
        sqlite3_bind_int(stmt, 1, c_num_insertion);

        int i = 0;

        while (sqlite3_step(stmt) != SQLITE_DONE)
        {
            i++;
        }

        sqlite3_finalize(stmt);

        ASSERT_EQ(0, i);
    };

    run_performance_test(simple_insert, "sqlite3::filter_no_match", c_num_iterations, c_num_insertion, true);
}

TEST_F(sqlite3_benchmark, filter_match)
{
    insert_records(db);

    auto simple_insert = [this]() {
        sqlite3_stmt* stmt = query(db, "SELECT * FROM simple_table WHERE uint64_field >= ?");
        sqlite3_bind_int(stmt, 1, c_num_insertion / 2);

        int i = 0;

        while (sqlite3_step(stmt) != SQLITE_DONE)
        {
            i++;
        }

        sqlite3_finalize(stmt);

        ASSERT_EQ(c_num_insertion / 2, i);
    };

    run_performance_test(simple_insert, fmt::format("sqlite3::filter_match {} matches", c_num_insertion / 2), c_num_iterations, c_num_insertion, true);
}

TEST_F(sqlite3_benchmark, simple_update)
{
    insert_records(db);

    auto simple_insert = [this]() {
        exec(db, "UPDATE simple_table SET uint64_field = uint64_field + 1");
    };

    run_performance_test(simple_insert, "sqlite3::simple_update", c_num_iterations, c_num_insertion, true);
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
