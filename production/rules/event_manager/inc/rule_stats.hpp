/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <atomic>
#include <string>

namespace gaia
{
namespace rules
{

// Performance counters for rules. For the rules engine scheduler, a single
// instance of this class (scheduler_stats_t inherits from rules_stats_t) is
// created and these stats are updated for all rules. If indvidual rule statistics
// are enabled, then an instance of this class per rule is created.
class rule_stats_t
{
public:
    rule_stats_t();
    rule_stats_t(const char* a_rule_id);

    std::string rule_id;
    std::string truncated_rule_id;
    std::atomic<uint32_t> count_executed;
    std::atomic<uint32_t> count_scheduled;
    std::atomic<uint32_t> count_pending;
    std::atomic<uint32_t> count_abandoned;
    std::atomic<uint32_t> count_retries;
    std::atomic<uint32_t> count_exceptions;
    std::atomic<int64_t> total_rule_invocation_latency;
    std::atomic<int64_t> total_rule_execution_time;
    std::atomic<int64_t> max_rule_invocation_latency;
    std::atomic<int64_t> max_rule_execution_time;

    void reset_counters();
    void add_rule_execution_time(int64_t duration);
    void add_rule_invocation_latency(int64_t duration);

    // Log individual rule stats and reset the counters.
    void log()
    {
        log(c_individual_stats_fmt, c_max_rule_id_len, truncated_rule_id.c_str());
    }

    // Log cumulative rule statistics for scheduler stats and reset the counters.
    void log(float worker_thread_utilization)
    {
        log(c_cumulative_stats_fmt, c_thread_load_padding, worker_thread_utilization);
    }

protected:
    // Ensure correct width padding to output a table with aligned columns.
    // Sample output (with some spacing removed for readability) is the following:
    //
    // [pattern]: --------------------- sched invoc  pend aband retry excep   avg lat   max lat   avg exec   max exec
    // [pattern]: [thread load: 0.05 %]    30    30     0     0     0     0   1.23 ms   3.00 ms    0.03 ms    0.24 ms
    // [pattern]: qualified_rule_name       1     1     0     0     0     0   1.08 ms   1.08 ms    0.03 ms    0.03 ms
    //
    // Where:
    // [pattern] is the user-defined logger pattern for the rule_stats logger defined in gaia_log.conf.
    // Row 1 is a header row (c_header_fmt).
    // Row 2 is a cumulative stats rule (c_cumulative_stats_fmt).
    // Row 3 is an individual rule stats row (c_individual_stats_fmt).
    static constexpr uint8_t c_max_rule_id_len = 30;
    static constexpr uint8_t c_thread_load_len = 17;
    static constexpr uint8_t c_thread_load_padding = c_max_rule_id_len - c_thread_load_len;
    static constexpr char c_header_fmt[] = "{:->{}}{: >6}{: >6}{: >6}{: >6}{: >6}{: >6}{: >13}{: >13}{: >13}{: >13}";
    static constexpr char c_individual_stats_fmt[] = "{: <{}}{:6}{:6}{:6}{:6}{:6}{:6}{:10.2f} ms{:10.2f} ms{:10.2f} ms{:10.2f} ms";
    static constexpr char c_cumulative_stats_fmt[] = "[thread load: {:{}.2f} %]{:6}{:6}{:6}{:6}{:6}{:6}{:10.2f} ms{:10.2f} ms{:10.2f} ms{:10.2f} ms";

private:
    template <typename T_param>
    void log(const char* stats_format, uint8_t width, T_param first_param);
};

} // namespace rules
} // namespace gaia
