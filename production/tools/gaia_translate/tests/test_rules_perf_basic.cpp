/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia/logger.hpp"
#include "gaia/rules/rules.hpp"

#include "gaia_internal/common/logger.hpp"
#include "gaia_internal/common/timer.hpp"
#include "gaia_internal/db/db_catalog_test_base.hpp"
#include "gaia_internal/rules/rules_test_helpers.hpp"

#include "gaia_perf_rules.h"
#include "test_perf.hpp"

using namespace gaia::perf_rules;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace std;
using namespace std::chrono;

atomic<uint32_t> g_c;
steady_clock::time_point g_in_rule;

static const size_t c_log_interval = 1;
const uint32_t c_single_rule_iterations = 100000;
const uint32_t c_multiple_rules_iterations = 100;
const bool c_enable_individual_rule_stats = true;
const bool c_timed = true;

class test_rules_perf_basic : public gaia::db::db_catalog_test_base_t
{
public:
    test_rules_perf_basic()
        : db_catalog_test_base_t("perf_rules.ddl"){};

    void SetUp() override
    {
        if (!s_is_initialized)
        {
            db_catalog_test_base_t::SetUp();
            s_is_initialized = true;
        }
    }

    void TearDown() override
    {
        clear_table<t1_t>();
    }

    void init_rules_engine(
        size_t num_rules_threads,
        bool enable_rule_stats = false)
    {
        gaia::rules::event_manager_settings_t settings;
        settings.num_background_threads = num_rules_threads;
        // Only enable cumulative stats, not individual rule stats.
        settings.enable_rule_stats = enable_rule_stats;
        // Log stats every second to get "Rules / Second"
        settings.stats_log_interval = 1;
        gaia::rules::test::initialize_rules_engine(settings);
        gaia::rules::unsubscribe_rules();
    }

    // We include the internal logger header so that we can write to the rule stats logger. This
    // enables us to group the statistics output by the test scenario.
    void log_scenario(
        const char* ruleset,
        size_t count_rules_threads)
    {
        static bool is_first_scenario = true;
        if (!is_first_scenario)
        {
            gaia_log::rules_stats().info("");
        }

        gaia_log::rules_stats().info("[scenario: ruleset '{}' executed with '{}' worker threads.]", ruleset, count_rules_threads);

        is_first_scenario = false;
    }

    void run_timed_insert_scenario(
        const char* ruleset,
        size_t count_rules_threads = s_num_hardware_threads)
    {
        run_insert_scenario(ruleset, 1, c_single_rule_iterations, count_rules_threads, !c_enable_individual_rule_stats, c_timed);
    }

    void run_insert_scenario(
        const char* ruleset,
        uint32_t expected_rules_count,
        size_t count_rules_threads = s_num_hardware_threads,
        bool enable_rule_stats = false)
    {
        run_insert_scenario(ruleset, expected_rules_count, c_multiple_rules_iterations, count_rules_threads, enable_rule_stats, !c_timed);
    }

    void run_insert_scenario(
        const char* ruleset,
        uint32_t expected_rules_count,
        uint32_t iterations,
        size_t count_rules_threads,
        bool enable_rule_stats,
        bool is_timed)
    {
        init_rules_engine(count_rules_threads, enable_rule_stats);
        gaia::rules::subscribe_ruleset(ruleset);
        log_scenario(ruleset, count_rules_threads);

        std::function<void()> insert_fn = [&]() {
            t1_t::insert_row(0);
        };

        is_timed ? run_timed_scenario(iterations, expected_rules_count, insert_fn) : run_scenario(iterations, expected_rules_count, insert_fn);
    }

    void run_timed_update_scenario(
        const char* ruleset,
        size_t count_rules_threads = s_num_hardware_threads)
    {
        run_update_scenario(ruleset, 1, c_single_rule_iterations, count_rules_threads, !c_enable_individual_rule_stats, c_timed);
    }

    void run_update_scenario(
        const char* ruleset,
        uint32_t expected_rules_count,
        size_t count_rules_threads = s_num_hardware_threads,
        bool enable_rule_stats = false)
    {
        run_update_scenario(ruleset, expected_rules_count, c_multiple_rules_iterations, count_rules_threads, enable_rule_stats, !c_timed);
    }

    void run_update_scenario(
        const char* ruleset,
        uint32_t expected_rules_count,
        uint32_t iterations,
        size_t count_rules_threads,
        bool enable_rule_stats,
        bool is_timed)
    {
        init_rules_engine(count_rules_threads, enable_rule_stats);
        gaia::db::begin_transaction();
        t1_t row = t1_t::get(t1_t::insert_row(0));
        gaia::db::commit_transaction();

        gaia::rules::subscribe_ruleset(ruleset);
        log_scenario(ruleset, count_rules_threads);

        std::function<void()> update_fn = [&]() {
            auto writer = row.writer();
            writer.a += 1;
            writer.update_row();
        };

        is_timed ? run_timed_scenario(iterations, expected_rules_count, update_fn) : run_scenario(iterations, expected_rules_count, update_fn);
    }

    // The timed scenario is run against rulesets that have a single rule in them.  Note that we are also
    // using the stats logger to get timings, but this offers a way to get a rough look at the numbers
    // in the console output against which we can double-check the rule stats logger numbers. Because
    // we wait for the rule to complete, these tests measure latency and not throughput (rules / second).
    void run_timed_scenario(uint32_t iterations, uint32_t expected_rules_count, std::function<void()> fn)
    {
        // Count of rules that have been invoked. Name kept short just to enable 10 rows per line in the rulesets
        // that have more than 10 rules each.
        g_c = 0;

        int64_t before_duration = 0;
        int64_t after_duration = 0;

        for (uint i = 0; i < iterations; ++i)
        {
            gaia::db::begin_transaction();
            fn();
            m_before_commit = gaia::common::timer_t::get_time_point();
            gaia::db::commit_transaction();
            m_after_commit = gaia::common::timer_t::get_time_point();
            // This will wait for the rule in the ruleset to fire but the 'g_in_rule' time_point
            // will be set before the rule is finished and the rules engine transaction
            // is committed so the time is measured right when the rule function is entered.
            gaia::rules::test::wait_for_rules_to_complete();
            before_duration += gaia::common::timer_t::get_duration(m_before_commit, g_in_rule);
            after_duration += gaia::common::timer_t::get_duration(m_after_commit, g_in_rule);
        }
        // Ensure we give enough time to the logger thread to write out
        // whatever stats it has before terminating the engine.
        sleep(c_log_interval * 2);
        gaia::rules::shutdown_rules_engine();

        // Just a sanity check to make sure we executed all the rules we scheduled.
        EXPECT_EQ(expected_rules_count * iterations, g_c);

        // The times reported here are overly optimistic (after_duration is too fast because it does not take into
        // account the commit overhead) and overly pessimistic (before_duration is too slow because it takes into account
        // all of the commit overhead and not just the time up to the point when a rule is scheduled).
        // The rule stats have a more accurate time as the duration is measured from the enqueing of the rule
        // in the commit_trigger to right before the rules engine invokes the rule. That said, these times are a nice
        // sanity check of the latencies recorded by the rule statistics: before_duration < rule_stats_latency < after_duration.
        printf("Latency to rule invocation (before commit, after commit): %0.2f us, %0.2f us\n", gaia::common::timer_t::ns_to_us(before_duration) / iterations, gaia::common::timer_t::ns_to_us(after_duration) / iterations);
    }

    // Non-timed scenarios run against rulesets with more than one rule. There's no easy way to capture
    // the latency over every rule short of rewriting the stats infrastructure. However, even for
    // "non-timed" scenarios, we are still capturing timing in the stats log. These tests only wait
    // for the rules to execute when the rules engine is shutdown. Therefore, these tests measure
    // throughput (how many rules can be scheduled and executed per second). In the stats log,
    // you'll see that the latency for each rule increases.  This is due to the fact that the rules
    // are sitting in the queue waiting to be invoked as the number of rules exceeds the number
    // of rules engine worker threads.
    void run_scenario(uint32_t iterations, uint32_t expected_rules_count, std::function<void()> fn)
    {
        g_c = 0;
        for (uint i = 0; i < iterations; ++i)
        {
            gaia::db::begin_transaction();
            fn();
            gaia::db::commit_transaction();
        }
        gaia::rules::test::wait_for_rules_to_complete();
        sleep(c_log_interval * 2);
        gaia::rules::shutdown_rules_engine();
        EXPECT_EQ(expected_rules_count * iterations, g_c);
    }

protected:
    static void SetUpTestSuite()
    {
        // User our own logger with the rule_stats log settings we want.  This will
        // deposit the log under [./logs/gaia_stats.log].
        db_test_base_t::SetUpTestSuite({"./gaia_log.conf"});
        s_num_hardware_threads = thread::hardware_concurrency();
    }
    static inline unsigned int s_num_hardware_threads = 1;

    steady_clock::time_point m_before_commit;
    steady_clock::time_point m_after_commit;

private:
    static inline bool s_is_initialized = false;
};

// These tests take a long time to run (minutes) so do not enable
// them. To run them, execute:
// ./test_rules_perf_basic --gtest_also_run_disabled_tests
//
// Results will be placed in: ./logs/gaia_stats.log
TEST_F(test_rules_perf_basic, DISABLED_insert_1_rule_1_thread)
{
    run_timed_insert_scenario("rules_insert", 1);
}

TEST_F(test_rules_perf_basic, DISABLED_insert_1_rule_max_thread)
{
    run_timed_insert_scenario("rules_insert");
}

TEST_F(test_rules_perf_basic, DISABLED_update_1_rule_1_thread)
{
    run_timed_update_scenario("rules_update", 1);
}

TEST_F(test_rules_perf_basic, DISABLED_update_1_rule_max_thread)
{
    run_timed_update_scenario("rules_update");
}

TEST_F(test_rules_perf_basic, DISABLED_insert_10)
{
    run_insert_scenario("rules_insert_10", 10);
}

TEST_F(test_rules_perf_basic, DISABLED_insert_10_serial_group)
{
    run_insert_scenario("rules_insert_10_serial", 10);
}

TEST_F(test_rules_perf_basic, DISABLED_insert_10_rule_stats)
{
    run_insert_scenario("rules_insert_10", 10, 1, s_num_hardware_threads, c_enable_individual_rule_stats, !c_timed);
}

TEST_F(test_rules_perf_basic, DISABLED_update_10)
{
    run_update_scenario("rules_update_10", 10);
}

TEST_F(test_rules_perf_basic, DISABLED_update_10_rule_stats)
{
    run_update_scenario("rules_update_10", 10, 1, s_num_hardware_threads, c_enable_individual_rule_stats, !c_timed);
}

TEST_F(test_rules_perf_basic, DISABLED_update_10_serial_group)
{
    run_update_scenario("rules_update_10_serial", 10);
}

TEST_F(test_rules_perf_basic, DISABLED_update_50)
{
    run_update_scenario("rules_update_50", 50);
}

TEST_F(test_rules_perf_basic, DISABLED_update_50_rule_stats)
{
    run_update_scenario("rules_update_50", 50, 1, s_num_hardware_threads, c_enable_individual_rule_stats, !c_timed);
}

TEST_F(test_rules_perf_basic, DISABLED_update_100)
{
    run_update_scenario("rules_update_100", 100);
}

TEST_F(test_rules_perf_basic, DISABLED_update_100_rule_stats)
{
    run_update_scenario("rules_update_100", 100, 1, s_num_hardware_threads, c_enable_individual_rule_stats, !c_timed);
}

TEST_F(test_rules_perf_basic, DISABLED_update_500)
{
    run_update_scenario("rules_update_500", 500);
}

TEST_F(test_rules_perf_basic, DISABLED_update_500_rule_stats)
{
    run_update_scenario("rules_update_500", 500, 1, s_num_hardware_threads, c_enable_individual_rule_stats, !c_timed);
}

TEST_F(test_rules_perf_basic, DISABLED_update_1000)
{
    run_update_scenario("rules_update_1000", 1000);
}

TEST_F(test_rules_perf_basic, DISABLED_update_1000_rule_stats)
{
    run_update_scenario("rules_update_1000", 1000, 1, s_num_hardware_threads, c_enable_individual_rule_stats, !c_timed);
}
