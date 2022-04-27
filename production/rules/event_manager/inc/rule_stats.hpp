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
    rule_stats_t(const std::string& a_rule_id);

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
    void log_individual();

    // Log cumulative rule statistics for scheduler stats and reset the counters.
    void log_cumulative(float worker_thread_utilization);

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
    // Row 1 is a header row (c_header_format).
    // Row 2 is a cumulative stats rule (c_cumulative_stats_format).
    // Row 3 is an individual rule stats row (c_individual_stats_format).
    static constexpr uint8_t c_max_rule_id_len = 30;
    static constexpr uint8_t c_thread_load_len = 17; // length of [thread load: ...] excluding the percentage
    static constexpr uint8_t c_max_row_len = 150;
    static constexpr uint8_t c_count_int_columns = 6;
    static constexpr uint8_t c_count_float_columns = 4;
    static constexpr uint8_t c_int_width = 10;
    static constexpr uint8_t c_float_width = 13; // includes len(' ms')

    static const char c_truncate_char = '~';
    static constexpr uint8_t c_thread_load_padding = c_max_rule_id_len - c_thread_load_len;
    static constexpr char c_header_format[] = "{:->{}}{: >10}{: >10}{: >10}{: >10}{: >10}{: >10}{: >13}{: >13}{: >13}{: >13}";
    static constexpr char c_individual_stats_format[] = "{: <{}}{:10}{:10}{:10}{:10}{:10}{:10}{:10.2f} ms{:10.2f} ms{:10.2f} ms{:10.2f} ms";
    static constexpr char c_cumulative_stats_format[] = "[{}{:{}.2f} %]{:10}{:10}{:10}{:10}{:10}{:10}{:10.2f} ms{:10.2f} ms{:10.2f} ms{:10.2f} ms";

    static constexpr char c_thread_load[] = "thread load: ";

    // Use these as column headings
    static constexpr char c_scheduled_column[] = "sched";
    static constexpr char c_invoked_column[] = "invoc";
    static constexpr char c_pending_column[] = "pend";
    static constexpr char c_abandoned_column[] = "aband";
    static constexpr char c_retries_column[] = "retry";
    static constexpr char c_exceptions_column[] = "excep";
    static constexpr char c_avg_latency_column[] = "avg lat";
    static constexpr char c_max_latency_column[] = "max lat";
    static constexpr char c_avg_execution_column[] = "avg exec";
    static constexpr char c_max_execution_column[] = "max exec";

    // Ensure width of [thread load: ...] padding does not exceed the maximum rule length.
    static_assert(c_max_rule_id_len >= c_thread_load_len, "Padding calculation for column widths must be >= 0!");

    // Ensure that the length of column headings does not exceed the widths of the columns themselves.
    static_assert(
        (sizeof(c_scheduled_column)
         + sizeof(c_invoked_column)
         + sizeof(c_pending_column)
         + sizeof(c_abandoned_column)
         + sizeof(c_retries_column)
         + sizeof(c_exceptions_column)
         + sizeof(c_avg_latency_column)
         + sizeof(c_max_latency_column)
         + sizeof(c_avg_execution_column)
         + sizeof(c_max_execution_column))
            < ((c_count_int_columns * c_int_width) + (c_count_float_columns * c_float_width)),
        "Column headers exceed maximum row length.");

    // Ensure the maximum name of the rule plus the widths of the columns does not exceed the maximum row length.
    static_assert(
        c_max_rule_id_len
                + (c_count_int_columns * c_int_width)
                + (c_count_float_columns * c_float_width)
            < c_max_row_len,
        "Maximum row length exceeded!");
};

} // namespace rules
} // namespace gaia
