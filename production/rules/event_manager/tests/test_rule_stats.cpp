/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <unistd.h>

#include <cstdio>

#include <iostream>

#include "gtest/gtest.h"
#include "spdlog/sinks/ostream_sink.h"

#include "debug_logger.hpp"
#include "logger_internal.hpp"
#include "rule_stats_manager.hpp"

using namespace gaia::common;
using namespace gaia::rules;
using namespace std;
using namespace std::chrono;

// The stats manager tests cover three classes: rule_stats_t, scheduler_stats_t,
// and the stats_manager_t.

// Helper data structure to populate statistics structures and expected results.
// Note that durations are expressed as floats so that they can be represented as
// fractional milliseconds to aid readability.
struct stats_data_t
{
    stats_data_t(
        const char* a_rule_id = nullptr,
        uint32_t a_scheduled = 0,
        uint32_t a_executed = 0,
        uint32_t a_pending = 0,
        uint32_t a_abandoned = 0,
        uint32_t a_retries = 0,
        uint32_t a_exceptions = 0,
        float a_avg_latency_ms = 0,
        float a_max_latency_ms = 0,
        float a_total_latency_ms = 0,
        float a_avg_exec_ms = 0,
        float a_max_exec_ms = 0,
        float a_total_exec_ms = 0,
        float a_total_thread_exec_ms = 0,
        float a_thread_load = 0)
        : rule_id(a_rule_id)
        , scheduled(a_scheduled)
        , executed(a_executed)
        , pending(a_pending)
        , abandoned(a_abandoned)
        , retries(a_retries)
        , exceptions(a_exceptions)
        , avg_latency_ms(a_avg_latency_ms)
        , max_latency_ms(a_max_latency_ms)
        , total_latency_ms(a_total_latency_ms)
        , avg_exec_ms(a_avg_exec_ms)
        , max_exec_ms(a_max_exec_ms)
        , total_exec_ms(a_total_exec_ms)
        , total_thread_exec_ms(a_total_thread_exec_ms)
        , thread_load(a_thread_load)
    {
    }
    const char* rule_id;
    uint32_t scheduled;
    uint32_t executed;
    uint32_t pending;
    uint32_t abandoned;
    uint32_t retries;
    uint32_t exceptions;
    float avg_latency_ms;
    float max_latency_ms;
    float total_latency_ms;
    float avg_exec_ms;
    float max_exec_ms;
    float total_exec_ms;
    float total_thread_exec_ms;
    float thread_load;
};

// Test helper to access protected members in rule_stats_t class.
class test_rule_stats_t : public rule_stats_t
{
public:
    static constexpr char c_dash[] = "-";

    // generate format strings in this class to get procteced access to widths.
    static uint8_t get_max_rule_id_len()
    {
        return c_max_rule_id_len;
    }

    static string get_int_format(bool as_string_specifier)
    {
        string format;
        string specifier = as_string_specifier ? "s" : "d";

        for (int i = 0; i < c_count_int_columns; i++)
        {
            format.append("%");
            format.append(to_string(c_int_width));
            format.append(specifier);
        }

        return format;
    }

    static string get_float_format(bool as_string_specifier)
    {
        string format;
        for (int i = 0; i < c_count_float_columns; i++)
        {
            format.append("%");
            if (as_string_specifier)
            {
                format.append(to_string(c_float_width));
                format.append("s");
            }
            else
            {
                format.append("10.2f ms");
            }
        }

        return format;
    }

    // Expected format of log result rows. There are three row types:
    // A header row, a cumulative statistics row, and an individual rule statistics row.
    static string get_header_format_string()
    {
        string format;
        bool as_string_specifier = true;

        for (int i = 0; i < test_rule_stats_t::get_max_rule_id_len(); i++)
        {
            format.append(c_dash);
        }

        format.append(get_int_format(as_string_specifier));
        format.append(get_float_format(as_string_specifier));

        return format;
    }

    static string get_individual_format_string()
    {
        bool as_number_format = false;
        string format = "%";
        format.append(c_dash);
        format.append(to_string(test_rule_stats_t::get_max_rule_id_len()));
        format.append("s");

        format.append(get_int_format(as_number_format));
        format.append(get_float_format(as_number_format));
        return format;
    }

    static string get_cumulative_format_string()
    {
        bool as_number_format = false;
        string format = "[";
        format.append(c_thread_load);
        format.append("%");
        format.append(to_string(c_thread_load_padding));
        format.append(".2f %]");
        format.append(get_int_format(as_number_format));
        format.append(get_float_format(as_number_format));
        return format;
    }

    static void make_header_row(char* buffer, size_t buffer_len)
    {
        snprintf(
            buffer, buffer_len, get_header_format_string().c_str(),
            c_scheduled_column, c_invoked_column, c_pending_column, c_abandoned_column, c_retries_column, c_exceptions_column,
            c_avg_latency_column, c_max_latency_column, c_avg_execution_column, c_max_execution_column);
    }

    static constexpr uint8_t get_max_line_len()
    {
        return c_max_row_len;
    }

    static constexpr uint8_t get_thread_load_len()
    {
        return c_thread_load_len;
    }

    static void truncate_rule_id(string rule_id)
    {
        rule_id[c_max_rule_id_len - 1] = c_truncate_char;
    }
};

// Test helper to access protected members in rule_stats_manager_t class.
class test_stats_manager_t : public rule_stats_manager_t
{
public:
    test_stats_manager_t(
        bool enable_rule_stats,
        size_t count_threads,
        uint32_t stats_log_interval)
        : rule_stats_manager_t(enable_rule_stats, count_threads, stats_log_interval)
    {
    }

    scheduler_stats_t& get_scheduler_stats()
    {
        return m_scheduler_stats;
    }

    std::map<string, rule_stats_t>& get_rule_stats_map()
    {
        return m_rule_stats_map;
    }

    void log_stats()
    {
        rule_stats_manager_t::log_stats();
    }

    uint8_t get_group_size()
    {
        return c_stats_group_size;
    }

    uint8_t get_count_entries_logged()
    {
        return m_count_entries_logged;
    }

    void set_count_entries_logged(uint8_t count)
    {
        m_count_entries_logged = count;
    }
};

class rule_stats_test : public ::testing::Test
{
public:
    // Do comparison with a big fudge factor (within 2x)
    // of expected value for time comparisons.  Note that all
    // times are in nanoseconds.  Equality comparison for
    // 0 values.
    void compare_duration(int64_t actual_ns, int64_t expected_ns)
    {
        if (expected_ns)
        {
            EXPECT_GT(actual_ns, expected_ns);
            EXPECT_LT(actual_ns, 2 * expected_ns);
        }
        else
        {
            EXPECT_EQ(actual_ns, 0);
        }
    }

    void fill_stats(rule_stats_t& stats, stats_data_t& data)
    {
        stats.count_scheduled = data.scheduled;
        stats.count_executed = data.executed;
        stats.count_pending = data.pending;
        stats.count_abandoned = data.abandoned;
        stats.count_retries = data.retries;
        stats.count_exceptions = data.exceptions;

        // All the durations are stored in nanoseconds even though the log
        // displays them in milliseconds.
        stats.total_rule_invocation_latency = ms_to_ns(data.total_latency_ms);
        stats.max_rule_invocation_latency = ms_to_ns(data.max_latency_ms);
        stats.total_rule_execution_time = ms_to_ns(data.total_exec_ms);
        stats.max_rule_execution_time = ms_to_ns(data.max_exec_ms);
    }

    void fill_stats(scheduler_stats_t& stats, stats_data_t& data)
    {
        stats.total_thread_execution_time = ms_to_ns(data.total_thread_exec_ms);
        fill_stats(static_cast<rule_stats_t&>(stats), data);
    }

    void verify_stats(const rule_stats_t& stats, const stats_data_t& expected)
    {
        EXPECT_STREQ(stats.rule_id.c_str(), expected.rule_id ? expected.rule_id : "");
        EXPECT_EQ(static_cast<uint32_t>(stats.count_exceptions), expected.exceptions);
        EXPECT_EQ(static_cast<uint32_t>(stats.count_executed), expected.executed);
        EXPECT_EQ(static_cast<uint32_t>(stats.count_pending), expected.pending);
        EXPECT_EQ(static_cast<uint32_t>(stats.count_retries), expected.retries);
        EXPECT_EQ(static_cast<uint32_t>(stats.count_abandoned), expected.abandoned);
        EXPECT_EQ(static_cast<uint32_t>(stats.count_scheduled), expected.scheduled);
        //cout << "max exec: " << stats.max_rule_execution_time << endl;
        compare_duration(static_cast<int64_t>(stats.max_rule_execution_time), ms_to_ns(expected.max_exec_ms));
        //cout << "tot exec: " << stats.total_rule_execution_time << endl;
        compare_duration(static_cast<int64_t>(stats.total_rule_execution_time), ms_to_ns(expected.total_exec_ms));
        //cout << "max invoc: " << stats.max_rule_invocation_latency << endl;
        compare_duration(static_cast<int64_t>(stats.max_rule_invocation_latency), ms_to_ns(expected.max_latency_ms));
        //cout << "tot invoc: " << stats.total_rule_invocation_latency << endl;
        compare_duration(static_cast<int64_t>(stats.total_rule_invocation_latency), ms_to_ns(expected.total_latency_ms));
    }

    void simulate_rule(test_stats_manager_t& manager, stats_data_t& data, float latency_ms, float exec_time_ms)
    {
        manager.insert_rule_stats(data.rule_id);

        if (data.scheduled)
        {
            manager.inc_scheduled(data.rule_id);
        };
        if (data.pending)
        {
            manager.inc_pending(data.rule_id);
        };
        if (data.abandoned)
        {
            manager.inc_abandoned(data.rule_id);
        };
        if (data.retries)
        {
            manager.inc_retries(data.rule_id);
        };
        if (data.exceptions)
        {
            manager.inc_exceptions(data.rule_id);
        };

        if (data.executed)
        {
            manager.inc_executed(data.rule_id);

            auto start_time_thread = steady_clock::now();
            usleep(ms_to_us(latency_ms));
            manager.compute_rule_invocation_latency(data.rule_id, start_time_thread);

            auto start_time_rule = steady_clock::now();
            usleep(ms_to_us(exec_time_ms));
            manager.compute_rule_execution_time(data.rule_id, start_time_rule);

            // Latency + execution time contributes to thread time.
            manager.compute_thread_execution_time(start_time_thread);
        }
    }

    void add_expected_line(const char* format, stats_data_t& data, bool add_header = false)
    {
        if (add_header)
        {
            test_rule_stats_t::make_header_row(m_line, c_line_length);
            m_expected_lines.emplace_back(m_line);
        }

        if (data.rule_id)
        {
            snprintf(m_line, c_line_length, format, data.rule_id, data.scheduled, data.executed, data.pending, data.abandoned, data.retries, data.exceptions, data.avg_latency_ms, data.max_latency_ms, data.avg_exec_ms, data.max_exec_ms);
        }
        else
        {
            snprintf(m_line, c_line_length, format, data.thread_load, data.scheduled, data.executed, data.pending, data.abandoned, data.retries, data.exceptions, data.avg_latency_ms, data.max_latency_ms, data.avg_exec_ms, data.max_exec_ms);
        }

        m_expected_lines.emplace_back(m_line);
    }

    // If use_fuzzy_match is true, then just match the first c_fuzzy_match_len characters of the line.
    void verify_log(bool use_fuzzy_match = false)
    {
        // Do the comparison of all the lines of the log with the logger stream.
        // Lines in the log are delimted by '\n'.
        string log_string = s_logger_output.str();

        for (auto const& expected_line : m_expected_lines)
        {
            auto len = log_string.find("\n");
            string log_line = log_string.substr(0, len);
            if (use_fuzzy_match)
            {
                string fuzzy_expected = expected_line.substr(0, c_fuzzy_match_len);
                string fuzzy_log = log_line.substr(0, c_fuzzy_match_len);
                EXPECT_EQ(fuzzy_log, fuzzy_expected);
            }
            else
            {
                EXPECT_EQ(log_line, expected_line);
            }
            log_string = log_string.substr(len + 1);
        }

        // Clear out the stream content.
        s_logger_output.str({});
        // Clear the output of the expected log
        m_expected_lines.clear();
    }

    string generate_rule_id(uint8_t length)
    {
        static constexpr char c_id[] = "Z";

        string rule_id;
        for (auto i = 0; i < length; i++)
        {
            rule_id.append(c_id);
        }
        return rule_id;
    }

    // Note that we are using floats to express fractional milliseconds.
    int64_t ms_to_ns(float duration)
    {
        return duration * 1e6;
    }

    int64_t ms_to_us(float duration)
    {
        return duration * 1e3;
    }

protected:
    static void SetUpTestSuite()
    {
        gaia_log::initialize({});

        // Use a synchronous debug logger which exposes the underlying spdlogger.  The
        // test uses an ostream sink and uses a custom pattern that only has the text we
        // want to log without a prefix (i.e., no timestamp, pid, or tid).
        gaia_log::debug_logger_t* debug_logger = gaia_log::debug_logger_t::create("test_logger");
        auto spdlogger = debug_logger->get_spdlogger();
        auto ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_st>(s_logger_output);
        ostream_sink->set_pattern("%v");
        spdlogger->sinks().emplace_back(ostream_sink);

        // Now set the underlying rule statistics logger.  Setting the logger will
        // transfer ownership of it to the gaia_log sub-system so don't delete
        // the debug_logger pointer here.
        gaia_log::set_rules_stats(debug_logger);
    }

    // The debug logger will write into this string stream
    // so that we can verify the log results.
    static ostringstream s_logger_output;

    // The contents of the logger are compared to a vector of lines since a single
    // statistics log call may output several lines. The comparision must match exactly
    // or just match a prefix ("fuzzy match").  Fuzzy matching is used to verifying log
    // when the test doesn't have control over the exact timings for durations.  Exact
    // match can be used when the test pre-fills the statistics values.
    static const size_t c_line_length = test_rule_stats_t::get_max_line_len();
    static const size_t c_fuzzy_match_len = test_rule_stats_t::get_thread_load_len();
    static const bool c_use_fuzzy_match = true;
    static const bool c_add_header = true;
    char m_line[c_line_length];
    vector<string> m_expected_lines;

    // Rule names used in testing invdividual rule statistics.
    const char* c_rule_id = "test_ruleset::0_employee";
    const char* c_rule1 = "rule1";
    const char* c_rule2 = "rule2";
    const char* c_rule3 = "rule3";
    const char* c_rule4 = "rule4";
    const char* c_rule5 = "rule5";
    const char* c_rule6 = "rule6";
};

ostringstream rule_stats_test::s_logger_output;

TEST_F(rule_stats_test, rule_stats_ctor)
{
    rule_stats_t stats(c_rule_id);
    verify_stats(stats, {c_rule_id});
}

TEST_F(rule_stats_test, rule_stats_null_rule_id)
{
    rule_stats_t stats(nullptr);
    EXPECT_STREQ(stats.rule_id.c_str(), "");
}

TEST_F(rule_stats_test, rule_stats_reset)
{
    rule_stats_t stats(c_rule_id);
    stats_data_t data = {c_rule_id, 1, 2, 3, 4, 5, 6};
    fill_stats(stats, data);
    stats.reset_counters();
    verify_stats(stats, {c_rule_id});
}

TEST_F(rule_stats_test, test_accumulators)
{
    rule_stats_t stats(c_rule_id);

    const int64_t c_max_latency = 90;
    int64_t latencies[] = {
        10,
        20,
        c_max_latency,
        40,
        50};

    const int64_t c_max_execution_time = 900;
    int64_t execution_times[] = {
        100,
        200,
        c_max_execution_time,
        400,
        500};

    int64_t total_latency = 0;
    for (auto latency : latencies)
    {
        stats.add_rule_invocation_latency(latency);
        total_latency += latency;
    }

    int64_t total_execution_time = 0;
    for (auto execution_time : execution_times)
    {
        stats.add_rule_execution_time(execution_time);
        total_execution_time += execution_time;
    }

    EXPECT_EQ(stats.total_rule_execution_time, total_execution_time);
    EXPECT_EQ(stats.total_rule_invocation_latency, total_latency);
    EXPECT_EQ(stats.max_rule_execution_time, c_max_execution_time);
    EXPECT_EQ(stats.max_rule_invocation_latency, c_max_latency);
}

TEST_F(rule_stats_test, test_log)
{
    rule_stats_t stats(c_rule_id);
    // Writes out stats from 1 - 10.  The (7*2) and (9*2) values represent
    // "total" times that when divided by the count_executed (2) yield the average
    // numbers 7 and 9 respectively.
    stats_data_t data = {c_rule_id, 1, 2, 3, 4, 5, 6, 7, 8, (7 * 2), 9, 10, (9 * 2)};
    fill_stats(stats, data);
    add_expected_line(test_rule_stats_t::get_individual_format_string().c_str(), data);
    stats.log_individual();
    verify_log();

    // Verify that stats are reset after a log call.
    data = {c_rule_id};
    add_expected_line(test_rule_stats_t::get_individual_format_string().c_str(), data);
    stats.log_individual();
    verify_log();
}

TEST_F(rule_stats_test, test_log_no_executions)
{
    rule_stats_t stats(c_rule_id);
    stats_data_t data = {c_rule_id, 5, 0, 4, 0, 0, 1};
    fill_stats(stats, data);
    add_expected_line(test_rule_stats_t::get_individual_format_string().c_str(), data);
    stats.log_individual();
    verify_log();
}

TEST_F(rule_stats_test, test_log_default)
{
    rule_stats_t stats;
    stats_data_t data = {""};
    fill_stats(stats, data);
    add_expected_line(test_rule_stats_t::get_individual_format_string().c_str(), data);
    stats.log_individual();
    verify_log();
}

TEST_F(rule_stats_test, test_log_cumulative)
{
    rule_stats_t stats;
    float thread_load = 25.0;

    stats_data_t data = {nullptr, 5, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, thread_load};
    fill_stats(stats, data);
    add_expected_line(test_rule_stats_t::test_rule_stats_t::get_cumulative_format_string().c_str(), data);
    stats.log_cumulative(thread_load);
    verify_log();

    // Verify that stats are reset after a log call.
    data = {};
    add_expected_line(test_rule_stats_t::get_cumulative_format_string().c_str(), data);
    stats.log_cumulative(0);
    verify_log();
}

TEST_F(rule_stats_test, test_scheduler_stats)
{
    const uint32_t log_interval = 10;
    const size_t count_threads = 2;

    scheduler_stats_t stats(log_interval, count_threads);
    verify_stats(stats, {});
    stats_data_t data = {nullptr, 1, 2, 3, 4, 5, 6, 7, 8, 14, 9, 10, 18, 5000, 25};
    fill_stats(stats, data);
    add_expected_line(test_rule_stats_t::get_cumulative_format_string().c_str(), data);
    stats.log(false);
    verify_log();

    verify_stats(stats, {});
}

TEST_F(rule_stats_test, test_scheduler_stats_header)
{
    const uint32_t log_interval = 10;
    const size_t count_threads = 2;

    scheduler_stats_t stats(log_interval, count_threads);
    stats_data_t data = {nullptr, 1, 2, 3, 4, 5, 6, 7, 8, 14, 9, 10, 18, 5000, 25};
    fill_stats(stats, data);
    add_expected_line(test_rule_stats_t::get_cumulative_format_string().c_str(), data, c_add_header);
    stats.log(c_add_header);
    verify_log();
}

TEST_F(rule_stats_test, test_stats_manager_header)
{
    const uint32_t log_interval = 0;
    const size_t count_threads = 10;
    bool enable_rule_stats = false;

    test_stats_manager_t manager(enable_rule_stats, count_threads, log_interval);
    verify_stats(manager.get_scheduler_stats(), {});

    // Since this is the first time we logged, we should have printed the header.
    stats_data_t data = {};
    add_expected_line(test_rule_stats_t::get_cumulative_format_string().c_str(), data, c_add_header);
    manager.log_stats();
    verify_log();

    // We should not see a header again until we log our group size
    uint8_t group_size = manager.get_group_size();
    while (manager.get_count_entries_logged() < group_size)
    {
        add_expected_line(test_rule_stats_t::get_cumulative_format_string().c_str(), data);
        manager.log_stats();
        verify_log();
    }

    // Once we hit our group size, we should log another header.
    add_expected_line(test_rule_stats_t::get_cumulative_format_string().c_str(), data, c_add_header);
    manager.log_stats();
    verify_log();
};

TEST_F(rule_stats_test, test_stats_manager_cumulative)
{
    const uint32_t log_interval = 0;
    const size_t count_threads = 1;
    bool enable_rule_stats = false;

    test_stats_manager_t manager(enable_rule_stats, count_threads, log_interval);
    scheduler_stats_t& stats = manager.get_scheduler_stats();

    // Test the cumulative stats through the test manager.  Individual rule
    // statistics have not been enabled in the stats_manager instance above.
    stats_data_t rule1 = {c_rule1, 1, 1}; // scheduled, executed
    stats_data_t rule2 = {c_rule2, 1, 1}; // scheduled, executed
    stats_data_t rule3 = {c_rule3, 0, 0, 1}; // pending
    stats_data_t rule4 = {c_rule4, 0, 0, 1, 1}; // pending, abandoned
    stats_data_t rule5 = {c_rule5, 1, 1, 0, 0, 1}; // scheduled, executed, retried
    stats_data_t rule6 = {c_rule6, 1, 1, 0, 0, 0, 1}; // scheduled, executed, exception

    // Simulate a rule running to gather statistics.  The
    // last two parameters are the latency to invoke the rule and the
    // execution time of the rule in milliseconds.  Rules 3 and 4 never
    // execute so they have no latency or exection durations.
    simulate_rule(manager, rule1, .5, .5);
    simulate_rule(manager, rule2, 1, 2);
    simulate_rule(manager, rule3, 0, 0);
    simulate_rule(manager, rule4, 0, 0);
    simulate_rule(manager, rule5, .5, 1);
    simulate_rule(manager, rule6, .5, .5);

    // Rollup of above since these are cumulative stats
    stats_data_t expected_stats = {nullptr, 4, 4, 2, 1, 1, 1, 0, 1, 2.5, 0, 2, 4, 6.5};
    verify_stats(stats, expected_stats);

    // Since we did not enable individual rule statistics, we should
    // not be tracking them.
    EXPECT_EQ(0, manager.get_rule_stats_map().size());

    // The log should have two lines in them: header and
    // a single row of cumulative stats.
    add_expected_line(test_rule_stats_t::get_cumulative_format_string().c_str(), expected_stats, c_add_header);
    manager.log_stats();
    verify_log(c_use_fuzzy_match);
}

TEST_F(rule_stats_test, test_stats_manager_multi_individual)
{
    const uint32_t log_interval = 0;
    const size_t count_threads = 1;
    bool enable_rule_stats = true;

    test_stats_manager_t manager(enable_rule_stats, count_threads, log_interval);
    scheduler_stats_t& stats = manager.get_scheduler_stats();

    // Test the cumulative stats through the test manager.  Individual rule
    // statistics have been enabled in the stats_manager instance above.
    stats_data_t rule1 = {c_rule1, 1, 1, 0, 0, 0, 0, 0, .5, .5, 0, .5, .5}; // scheduled, executed
    stats_data_t rule2 = {c_rule2, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 2, 2}; // scheduled, executed
    stats_data_t rule3 = {c_rule3, 0, 0, 1}; // pending
    stats_data_t rule4 = {c_rule4, 0, 0, 1, 1}; // pending, abandoned
    stats_data_t rule5 = {c_rule5, 1, 1, 0, 0, 1, 0, 0, .5, .5, 0, 1, 1}; // scheduled, executed, retried
    stats_data_t rule6 = {c_rule6, 1, 1, 0, 0, 0, 1, 0, .5, .5, 0, .5, .5}; // scheduled, executed, exception

    // Simulate a rule running to gather statistics.  The
    // last two parameters are the latency to invoke the rule and the
    // execution time of the rule in milliseconds.  Rules 3 and 4 never
    // execute so they have no latency or execution durations.
    simulate_rule(manager, rule1, .5, .5);
    simulate_rule(manager, rule2, 1, 2);
    simulate_rule(manager, rule3, 0, 0);
    simulate_rule(manager, rule4, 0, 0);
    simulate_rule(manager, rule5, .5, 1);
    simulate_rule(manager, rule6, .5, .5);

    // Rollup of above since these are cumulative stats
    stats_data_t cumulative_stats = {nullptr, 4, 4, 2, 1, 1, 1, 0, 1, 2.5, 0, 2, 4, 6.5};
    verify_stats(stats, cumulative_stats);

    // Now verify each of the indivual rule stats
    auto& stats_map = manager.get_rule_stats_map();
    EXPECT_EQ(6, stats_map.size());
    verify_stats(stats_map[c_rule1], rule1);
    verify_stats(stats_map[c_rule2], rule2);
    verify_stats(stats_map[c_rule3], rule3);
    verify_stats(stats_map[c_rule4], rule4);
    verify_stats(stats_map[c_rule5], rule5);
    verify_stats(stats_map[c_rule6], rule6);

    add_expected_line(test_rule_stats_t::get_cumulative_format_string().c_str(), cumulative_stats, c_add_header);
    add_expected_line(test_rule_stats_t::get_individual_format_string().c_str(), rule1);
    add_expected_line(test_rule_stats_t::get_individual_format_string().c_str(), rule2);
    add_expected_line(test_rule_stats_t::get_individual_format_string().c_str(), rule3);
    add_expected_line(test_rule_stats_t::get_individual_format_string().c_str(), rule4);
    add_expected_line(test_rule_stats_t::get_individual_format_string().c_str(), rule5);
    add_expected_line(test_rule_stats_t::get_individual_format_string().c_str(), rule6);
    manager.log_stats();
    verify_log(c_use_fuzzy_match);
}

TEST_F(rule_stats_test, test_stats_manager_same_individual)
{
    const uint32_t log_interval = 0;
    const size_t count_threads = 1;
    bool enable_rule_stats = true;

    test_stats_manager_t manager(enable_rule_stats, count_threads, log_interval);
    scheduler_stats_t& stats = manager.get_scheduler_stats();

    // Test the cumulative stats through the test manager.  Individual rule
    // statistics have not been enabled in the stats_manager instance above.
    stats_data_t rule1 = {c_rule1, 1, 1}; // scheduled, executed

    // Simulate a rule running to gather statistics.  The
    // last two parameters are the latency to invoke the rule and the
    // execution time of the rule in milliseconds.  Rules 3 and 4 never
    // execute so they have no latency or exection durations.
    simulate_rule(manager, rule1, .5, .5);
    simulate_rule(manager, rule1, 1, 2);

    // Rollup of above since these are cumulative stats
    stats_data_t expected_stats = {nullptr, 2, 2, 0, 0, 0, 0, 0, 1, 1.5, 0, 2, 2.5};
    verify_stats(stats, expected_stats);

    // We only have statistics for rule1 which should be the same as the overall stats
    // except that it will have a filled in rule_id.
    auto& stats_map = manager.get_rule_stats_map();
    EXPECT_EQ(1, stats_map.size());

    expected_stats.rule_id = c_rule1;
    verify_stats(stats_map[c_rule1], expected_stats);

    add_expected_line(test_rule_stats_t::get_cumulative_format_string().c_str(), expected_stats, c_add_header);
    add_expected_line(test_rule_stats_t::get_individual_format_string().c_str(), expected_stats);
    manager.log_stats();
    verify_log(c_use_fuzzy_match);
}

TEST_F(rule_stats_test, test_truncated_rule_id)
{
    // An empty rule name should be okay.
    rule_stats_t empty_name;
    string expected_rule_id;
    EXPECT_EQ(expected_rule_id, empty_name.rule_id);
    EXPECT_EQ(expected_rule_id, empty_name.truncated_rule_id);

    // A rule name <= max_rule_id_len should not be truncated.
    expected_rule_id = generate_rule_id(test_rule_stats_t::get_max_rule_id_len());
    rule_stats_t name_equals_limit(expected_rule_id.c_str());
    EXPECT_EQ(expected_rule_id, name_equals_limit.rule_id);
    EXPECT_EQ(expected_rule_id, name_equals_limit.truncated_rule_id);

    // A rule name > max_rule_id_len should be truncated with a '~' character.
    expected_rule_id = generate_rule_id(test_rule_stats_t::get_max_rule_id_len() + 10);
    rule_stats_t name_exceeds_limit(expected_rule_id.c_str());
    EXPECT_EQ(expected_rule_id, name_exceeds_limit.rule_id);

    expected_rule_id = generate_rule_id(test_rule_stats_t::get_max_rule_id_len() - 1);
    test_rule_stats_t::truncate_rule_id(expected_rule_id);
    EXPECT_EQ(expected_rule_id, name_exceeds_limit.truncated_rule_id);
}
