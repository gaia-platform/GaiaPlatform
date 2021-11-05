/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <unistd.h>

#include <chrono>
#include <cstring>
#include <ctime>

#include <algorithm>
#include <atomic>
#include <string>
#include <thread>

#include "gaia/logger.hpp"
#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"

#include "simulation_exceptions.hpp"

uint64_t generate_unique_millisecond_timestamp();
extern std::atomic<uint64_t> g_timestamp;
extern std::atomic<int> g_rule_1_tracker;

class simulation_t
{
public:
    // Main entry point.
    int main(int argc, const char** argv);

protected:
    typedef std::chrono::steady_clock my_clock_t;
    typedef my_clock_t::time_point my_time_point_t;
    typedef std::chrono::duration<double, std::micro> my_duration_in_microseconds_t;

private:
    // Used for conversions.
    const double c_milliseconds_in_second = 1000.0;
    const double c_microseconds_in_second = 1000000.0;

    // For any of the pauses, microseconds between checks
    const int c_processing_pause_in_microseconds = 10;
    const int c_normal_wait_timeout_in_microseconds = 3000;

    // Defaul timeout to use when waiting for actions to complete.
    const long c_default_processing_complete_timeout_in_microseconds = static_cast<long>(300 * c_microseconds_in_second);

    // Main menu commands.
    static constexpr char c_cmd_toggle_measurement = 'o';
    static constexpr char c_cmd_toggle_pause_mode = 'p';
    static constexpr char c_cmd_toggle_debug_mode = 'D';
    static constexpr char c_cmd_comment = '#';
    static constexpr char c_cmd_wait = 'w';
    static constexpr char c_cmd_test = 't';
    static constexpr char c_cmd_initialize = 'i';
    static constexpr char c_cmd_state = 's';
    static constexpr char c_cmd_step_sim = 'z';

    // Collected times of various points in the simulation.
    double m_explicit_wait_time_in_microseconds = 0.0;
    double m_check_time_in_microseconds = 0.0;
    double m_total_wait_time_in_microseconds = 0.0;
    double m_total_print_time_in_microseconds = 0.0;
    double m_total_start_transaction_duration_in_microseconds = 0.0;
    double m_total_inside_transaction_duration_in_microseconds = 0.0;
    double m_total_end_transaction_duration_in_microseconds = 0.0;
    double m_update_row_duration_in_microseconds = 0.0;
    my_duration_in_microseconds_t m_measured_duration_in_microseconds = my_duration_in_microseconds_t(0);

    // This keep track of a separate log file used for debugging, independant of the
    // other logs.
    //
    // Currently, it is used to track the values of the rule trackers for later examination
    // and verification of the stats logs.
    //
    // This logging can be turned on and off using the `D` command, and will be
    // automatically closed at the end of the application.  When closed, it will be
    // assigned to nullptr.  When open, it will have the File * that is the output file.
    FILE* m_debug_log_file = nullptr;
    const int c_rules_firing_update_buffer_length = 4096;

    // After the simulation ends, the number of seconds to wait before doing
    // the final summary for the simulation, including the final output of
    // the gaia_stats.log.
    //
    // This is in place to give the simulation a good chance at capturing all
    // relevant debug information.
    const int c_default_sleep_time_in_seconds_after_stop = 6;
    int m_sleep_time_in_seconds_after_stop;

    // When emitting state, to track whether anything has been emitted
    // so the proper join sequence with the next output can be emitted.
    bool m_has_intermediate_state_output = false;

    // File to emit any state information to.
    FILE* m_object_log_file = nullptr;

    // Used to get a more accurate measurement using the toggle on/off (o) command.
    bool m_is_measured_duration_timer_on = false;
    bool m_have_measurement = false;
    my_time_point_t m_measured_duration_start_mark;

    // Count of the number of iterations through the test loop that were executed.
    int m_number_of_test_iterations = 0;

    // Current line read from the commands.txt file.
    std::string m_command_input_line;

    // Make sure we track if the database has been initialized, or if we still need
    // to do it.
    bool m_has_database_been_initialized = false;

    // The pause mode, either a transaction based wait or an atomic based wait.
    bool m_use_transaction_wait = true;

private:
    void toggle_measurement();
    void toggle_debug_mode();
    void toggle_pause_mode();

    void wait_for_test_processing_to_complete(bool is_explicit_pause, long timeout_in_microseconds);

    bool read_input();
    void close_open_log_files();
    void sleep_for(long parse_for_microsecond);
    void handle_wait();
    bool handle_main();

    void handle_test(const std::string& input);
    void handle_initialize(const std::string& input);
    void display_usage(const char* command);
    int handle_command_line_arguments(int argc, const char** argv);

    int run_simulation();
    void dump_exception_stack();
    void handle_multiple_steps(const std::string& input);
    void simulation_step();
    int wait_for_step_processing_to_complete(bool is_explicit_pause, int initial_state_tracker, int timeout_in_microseconds);

protected:
    bool get_use_transaction_wait();

    // Number of microseconds to wait before declaring the test dead.
    //
    // Defaults to c_default_processing_complete_timeout_in_microseconds.
    virtual long get_processing_timeout_in_microseconds();

    // Dump the current state of the database, in JSON format, to the specified file.
    virtual void dump_db_json(FILE* object_log_file);

    // Local name of the configuration file to execute the test against.
    //
    // Note that this is typically the name of the workload + ".conf", as set up
    // by the suite.sh file.
    virtual std::string get_configuration_file_name() = 0;

    // Initialize any database storage to the clean state, with only constant elements
    // in the database.
    virtual void init_storage() = 0;

    // Setup the data for test test to execute for the specified number of iterations.
    //
    // Note that as these tests are almost all database driven, this function will
    // most likely involve openning a transaction and setting a goal that the simulation
    // will work towards.
    virtual void setup_test_data(int required_iterations) = 0;

    // Return true if the test has completed.
    //
    // Note that as these tests are almost all database driven, this function will
    // most likely involve opening a transaction, searching for a given object or
    // objects in the database, and checking to see if they are in their final state.
    virtual bool has_test_completed() = 0;

    // For single step tests, perform a single step.
    virtual my_duration_in_microseconds_t perform_single_step() = 0;

    // For single step tests, note when that single step has completed.
    virtual bool has_step_completed(int timestamp, int initial_state_tracker) = 0;
};
