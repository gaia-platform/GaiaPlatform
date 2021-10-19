/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <unistd.h>

#include <chrono>
#include <cstring>
#include <ctime>

#include <algorithm>
#include <atomic>
#include <iostream>
#include <string>
#include <thread>

#include "gaia/logger.hpp"
#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"

#include "gaia_palletbox.h"

#include "constants.hpp"
#include "exceptions.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::direct_access;
using namespace gaia::palletbox;
using namespace gaia::palletbox::exceptions;
using namespace gaia::rules;

using namespace exceptions;

typedef std::chrono::steady_clock my_clock;
typedef my_clock::time_point my_time_point;
typedef std::chrono::microseconds microseconds;
typedef std::chrono::duration<double, std::micro> my_duration_in_microseconds;


static constexpr int c_sole_robot_id = 1;

static constexpr char c_sandbox_station_charging[] = "charging";
static constexpr char c_sandbox_station_outbound[] = "outbound";
static constexpr char c_sandbox_station_inbound[] = "inbound";

namespace gaia
{
namespace palletbox
{


std::unordered_map<int, const char*> station_name_map = {
    {(int)station_id_t::charging, c_sandbox_station_charging},
    {(int)station_id_t::outbound, c_sandbox_station_outbound},
    {(int)station_id_t::inbound, c_sandbox_station_inbound}
};

} // namespace palletbox
} // namespace gaia


// Common Names
const char* g_configuration_file_name = "palletbox.conf";

// Used for conversions.
const double c_milliseconds_in_second = 1000.0;
const double c_microseconds_in_second = 1000000.0;

// Collected times of various points in the simulation.
double g_explicit_wait_time_in_microseconds = 0.0;
double g_check_time_in_microseconds = 0.0;
double g_total_wait_time_in_microseconds = 0.0;
double g_total_print_time_in_microseconds = 0.0;
double g_total_start_transaction_duration_in_microseconds = 0.0;
double g_total_inside_transaction_duration_in_microseconds = 0.0;
double g_total_end_transaction_duration_in_microseconds = 0.0;
double g_update_row_duration_in_microseconds = 0.0;
my_duration_in_microseconds g_measured_duration_in_microseconds;

// for any of the pauses, microseconds between checks
const int c_processing_pause_in_microseconds = 10;

// Timeouts to use when waiting for actions to complete.
const int c_normal_wait_timeout_in_microseconds = 3000;
const int c_marco_polo_wait_timeout_in_microseconds = static_cast<int>(300 * c_microseconds_in_second);

/*
This keep track of a separate log file used for debugging, independant of the
other logs.

Currently, it is used to track the values of the rule trackers for later examination
and verification of the stats logs.

This logging can be turned on and off using the `D` command, and will be
automatically closed at the end of the application.  When closed, it will be
assigned to nullptr.  When open, it will have the File * that is the output file.
*/
FILE* g_debug_log_file = nullptr;
const int c_rules_firing_update_buffer_length = 4096;

// after the simulation ends, the number of seconds to wait before doing
// the final summary for the simulation, including the final output of
// the gaia_stats.log.
//
// this is in place to give the simulation a good chance at capturing all
// relevant debug information.
const int c_default_sleep_time_in_seconds_after_stop = 6;
int g_sleep_time_in_seconds_after_stop;

// When emitting state, to track whether anything has been emitted
// so the proper join sequence with the next output can be emitted.
bool g_has_intermediate_state_output = false;

atomic<int> g_rule_1_tracker{0};

// File to emit any state information to.
FILE* g_object_log_file = nullptr;

// Used to get a more accurate measurement using the toggle on/off (o) command.
bool g_is_measured_duration_timer_on;
bool g_have_measurement;
my_time_point g_measured_duration_start_mark;

// Forward References
int wait_for_processing_to_complete(bool is_explicit_pause, int timeout);

atomic<uint64_t> g_timestamp{1};

uint64_t get_time_millis()
{
    return g_timestamp++;
}

void my_sleep_for(long parse_for_microsecond)
{
    my_time_point start_mark = my_clock::now();
    while (chrono::duration_cast<chrono::microseconds>(my_clock::now() - start_mark).count() < parse_for_microsecond)
    {
        ;
    }
}

gaia_id_t insert_station(station_id_t station_id)
{
    station_writer w;
    w.id = (int)station_id;
    return w.insert_row();
}

gaia_id_t insert_robot(int robot_id)
{
    robot_writer w;
    w.id = robot_id;
    w.times_to_charging = 0;
    w.target_times_to_charge = 1;
    printf("--->insert row\n");
    gaia_id_t gid = w.insert_row();
    printf("--->row inserted\n");
    return gid;
}

void restore_station_table()
{
    for (auto& station_table : station_t::list())
    {
        station_writer w = station_table.writer();
        w.update_row();
    }
}

void restore_robot_table(robot_t& robot_table, int required_iterations)
{
    robot_writer w = robot_table.writer();
    w.times_to_charging = 0;
    w.target_times_to_charge = required_iterations;
    //w.station_id = (int)station_id_t::charging;
    w.update_row();
}

void restore_default_values(int expected_iterations)
{
    restore_station_table();
    for (auto& robot_table : robot_t::list())
    {
        restore_robot_table(robot_table, expected_iterations);
    }
}

void init_storage()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);

    // If we already have inserted a ping pong table then our storage has already been
    // initialized.  Re-initialize the database to default values.
    if (station_t::list().size())
    {
        restore_default_values(-1);
        tx.commit();
        return;
    }

    auto charging_station = station_t::get(insert_station(station_id_t::charging));
    insert_station(station_id_t::outbound);
    insert_station(station_id_t::inbound);

    auto pallet_bot = robot_t::get(insert_robot(c_sole_robot_id));
    charging_station.robots().connect(pallet_bot);
    tx.commit();
}

void dump_db_json()
{
    begin_transaction();
    fprintf(g_object_log_file, "{\n");
    bool is_first = true;
    fprintf(g_object_log_file, "  \"x\" : {");
    for (const gaia::palletbox::station_t& i : station_t::list())
    {
        if (is_first)
        {
            is_first = false;
        }
        else
        {
            fprintf(g_object_log_file, ",\n");
        }
        fprintf(g_object_log_file, "  \"%d\" : {\n", i.id());
        fprintf(g_object_log_file, "  }");
    }
    fprintf(g_object_log_file, "},\n  \"y\" : {");
    fprintf(g_object_log_file, "}\n");
    commit_transaction();
}

void toggle_measurement()
{
    if (!g_is_measured_duration_timer_on)
    {
        printf("Measure timer activated.\n");
        g_is_measured_duration_timer_on = true;
        g_measured_duration_start_mark = my_clock::now();
    }
    else
    {
        my_time_point measured_duration_end_mark = my_clock::now();
        g_is_measured_duration_timer_on = false;
        g_have_measurement = true;
        g_measured_duration_in_microseconds = measured_duration_end_mark - g_measured_duration_start_mark;
        printf("Measure timer deactivated.\n");
    }
}

void toggle_debug_mode()
{
    if (g_debug_log_file == nullptr)
    {
        printf("Debug log activated.\n");
        g_debug_log_file = fopen("test-results/debug.log", "w");
    }
    else
    {
        fclose(g_debug_log_file);
        printf("Debug log deactivated.\n");
    }
}

int handle_test(string m_input)
{
    int limit = stoi(m_input.substr(1, m_input.size() - 1));

    my_time_point start_transaction_start_mark = my_clock::now();
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    my_time_point inside_transaction_start_mark = my_clock::now();

    restore_default_values(limit);

    auto robot_iter = robot_t::list().where(robot_expr::id == c_sole_robot_id).begin();
    if (robot_iter == robot_t::list().end())
    {
        throw palletbox_exception(gaia_fmt::format("Cannot find robot with id {}", c_sole_robot_id));
    }

    gaia_id_t ff = bot_moving_to_station_event_t::insert_row(get_time_millis(), (int)station_id_t::inbound, robot_iter->id());
    // auto new_event = bot_moving_to_station_event_t::get(ff);
    // robot_iter->bot_moving_to_station_events().connect(new_event);
    // robot_iter->station().bot_moving_to_station_events().connect(new_event);

    my_time_point inside_transaction_end_mark = my_clock::now();
    tx.commit();
    my_time_point commit_transaction_end_mark = my_clock::now();

    wait_for_processing_to_complete(false, c_marco_polo_wait_timeout_in_microseconds);

    my_duration_in_microseconds start_transaction_duration = inside_transaction_start_mark - start_transaction_start_mark;
    my_duration_in_microseconds inside_transaction_duration = inside_transaction_end_mark - inside_transaction_start_mark;
    my_duration_in_microseconds end_transaction_duration = commit_transaction_end_mark - inside_transaction_end_mark;
    g_total_start_transaction_duration_in_microseconds += start_transaction_duration.count();
    g_total_inside_transaction_duration_in_microseconds += inside_transaction_duration.count();
    g_total_end_transaction_duration_in_microseconds += end_transaction_duration.count();
    return limit;
}

bool has_completed()
{
    int times_to_charging = -1;
    int target_times_to_charge = -2;
    begin_transaction();
    for (const gaia::palletbox::robot_t& i : robot_t::list())
    {
        times_to_charging = i.times_to_charging();
        target_times_to_charge = i.target_times_to_charge();
    }
    commit_transaction();

    return times_to_charging == target_times_to_charge;
}

int wait_for_processing_to_complete(bool is_explicit_pause, int timeout_in_microseconds)
{
    my_time_point end_sleep_start_mark = my_clock::now();

    const int maximum_no_delta_attempts = timeout_in_microseconds / c_processing_pause_in_microseconds;

    int timestamp = g_timestamp;

    char trace_buffer[c_rules_firing_update_buffer_length];
    char update_trace_buffer[c_rules_firing_update_buffer_length];
    int space_left = sizeof(update_trace_buffer);
    char* start_pointer = update_trace_buffer;
    int amount_written = 0;

    int current_no_delta_attempt;

    for (current_no_delta_attempt = 0;
         current_no_delta_attempt < maximum_no_delta_attempts;
         current_no_delta_attempt++)
    {
        my_time_point check_start_mark = my_clock::now();
        bool have_completed = has_completed();
        my_time_point check_end_mark = my_clock::now();
        my_duration_in_microseconds ms_double = check_end_mark - check_start_mark;
        g_check_time_in_microseconds += ms_double.count();

        if (g_debug_log_file != nullptr)
        {
            int new_amount_written = snprintf(start_pointer, space_left, "%d,", static_cast<int>(have_completed));
            space_left -= new_amount_written;
            start_pointer += new_amount_written;
            amount_written += new_amount_written;
        }
        if (have_completed)
        {
            break;
        }
        my_sleep_for(c_processing_pause_in_microseconds);
    }
    my_time_point end_sleep_end_mark = my_clock::now();
    my_duration_in_microseconds ms_double = end_sleep_end_mark - end_sleep_start_mark;

    if (current_no_delta_attempt == maximum_no_delta_attempts)
    {
        printf("\nwait_for_processing_to_complete -- TIMEOUT!!!\n");
    }

    if (is_explicit_pause)
    {
        g_explicit_wait_time_in_microseconds += ms_double.count();
    }
    else
    {
        g_total_wait_time_in_microseconds += ms_double.count();
    }
    if (g_debug_log_file != nullptr)
    {
        int new_amount_written = snprintf(trace_buffer, sizeof(trace_buffer), "%d,%.9f,%s\n", timestamp, ms_double.count(), update_trace_buffer);
        fwrite(trace_buffer, 1, new_amount_written, g_debug_log_file);
    }
    return current_no_delta_attempt;
}

void usage(const char* command)
{
    printf("Usage: %s debug <input-file>\n", command);
}

class simulation_t
{
public:
    // Main menu commands.
    static constexpr char c_cmd_wait = 'w';

    static constexpr char c_cmd_test = 't';

    static constexpr char c_cmd_toggle_measurement = 'o';
    static constexpr char c_cmd_toggle_pause_mode = 'p';
    static constexpr char c_cmd_toggle_debug_mode = 'D';

    static constexpr char c_cmd_comment = '#';

    int last_known_timestamp = 0;

    void stop()
    {
        last_known_timestamp = g_timestamp;
        if (g_debug_log_file != nullptr)
        {
            fclose(g_debug_log_file);
            g_debug_log_file = nullptr;
        }
    }

    void my_sleep_for(long parse_for_microsecond)
    {
        my_time_point start_mark = my_clock::now();
        while (chrono::duration_cast<chrono::microseconds>(my_clock::now() - start_mark).count() < parse_for_microsecond)
        {
            ;
        }
    }

    void handle_wait()
    {
        my_time_point end_sleep_start_mark = my_clock::now();

        int limit = stoi(m_input.substr(1, m_input.size() - 1));
        std::this_thread::sleep_for(microseconds(limit * (static_cast<long>(c_microseconds_in_second) / static_cast<long>(c_milliseconds_in_second))));

        my_time_point end_sleep_end_mark = my_clock::now();

        my_duration_in_microseconds ms_double = end_sleep_end_mark - end_sleep_start_mark;
        g_explicit_wait_time_in_microseconds += ms_double.count();
    }

    // Return false if EOF is reached.
    bool read_input()
    {
        getline(cin, m_input);
        return !cin.eof();
    }

    bool handle_main()
    {
        if (!read_input())
        {
            // Stop the simulation as well.
            return false;
        }

        printf("Input: %s\n", m_input.c_str());
        if (m_input.size() == 1)
        {
            switch (m_input[0])
            {
            case c_cmd_toggle_measurement:
                toggle_measurement();
                break;
            case c_cmd_toggle_debug_mode:
                toggle_debug_mode();
                break;
            default:
                printf("Wrong input: %c\n", m_input[0]);
                break;
            }
        }
        else if (m_input.size() > 1 && (m_input[0] == c_cmd_wait || m_input[0] == c_cmd_comment || m_input[0] == c_cmd_test))
        {
            if (m_input[0] == c_cmd_wait)
            {
                handle_wait();
            }
            else if (m_input[0] == c_cmd_comment)
            {
                ;
            }
            else if (m_input[0] == c_cmd_test)
            {
                g_timestamp = handle_test(m_input);
            }
        }
        return true;
    }

    int run()
    {
        bool has_input = true;
        while (has_input)
        {
            has_input = handle_main();
        }

        if (g_has_intermediate_state_output)
        {
            fprintf(g_object_log_file, "]\n");
        }

        stop();
        my_time_point end_sleep_start_mark = my_clock::now();
        sleep(g_sleep_time_in_seconds_after_stop);
        my_time_point end_sleep_end_mark = my_clock::now();
        my_duration_in_microseconds ms_double = end_sleep_end_mark - end_sleep_start_mark;

        const int c_measured_buffer_size = 100;
        char measured_buffer[c_measured_buffer_size];
        measured_buffer[0] = '\0';
        if (g_have_measurement)
        {
            double measured_in_secs = g_measured_duration_in_microseconds.count() / c_microseconds_in_second;
            snprintf(measured_buffer, sizeof(measured_buffer), ", \"measured_in_sec\" : %.9f", measured_in_secs);
        }

        FILE* delays_file = fopen("test-results/output.delay", "w");
        fprintf(delays_file, "{ \"stop_pause_in_sec\" : %.9f, "
                             "\"iterations\" : %d, "
                             "\"start_transaction_in_sec\" : %.9f, "
                             "\"inside_transaction_in_sec\" : %.9f, "
                             "\"end_transaction_in_sec\" : %.9f, "
                             "\"update_row_in_sec\" : %.9f, "
                             "\"total_wait_in_sec\" : %.9f, "
                             "\"explicit_wait_in_sec\" : %.9f, "
                             "\"check_time_in_sec\" : %.9f, "
                             "\"total_print_in_sec\" : %.9f"
                             "%s }\n",
                ms_double.count() / c_microseconds_in_second, last_known_timestamp, g_total_start_transaction_duration_in_microseconds / c_microseconds_in_second, g_total_inside_transaction_duration_in_microseconds / c_microseconds_in_second, g_total_end_transaction_duration_in_microseconds / c_microseconds_in_second, g_update_row_duration_in_microseconds / c_microseconds_in_second, g_total_wait_time_in_microseconds / c_microseconds_in_second, g_explicit_wait_time_in_microseconds / c_microseconds_in_second, g_check_time_in_microseconds / c_microseconds_in_second, g_total_print_time_in_microseconds / c_microseconds_in_second, measured_buffer);
        fclose(delays_file);
        return EXIT_SUCCESS;
    }

private:
    string m_input;
};

int main(int argc, const char** argv)
{
    const char* c_arg_debug = "debug";

    if (argc >= 2 && strncmp(argv[1], c_arg_debug, strlen(c_arg_debug)) == 0)
    {
        if (argc == 3)
        {
            g_sleep_time_in_seconds_after_stop = atoi(argv[2]);
        }
        else
        {
            g_sleep_time_in_seconds_after_stop = c_default_sleep_time_in_seconds_after_stop;
        }
    }
    else
    {
        if (argc > 1)
        {
            cout << "Wrong arguments." << endl;
        }
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    int return_code = EXIT_FAILURE;
    simulation_t sim;
    try
    {
        g_object_log_file = fopen("test-results/output.json", "w");

        printf("-->Initializeing system\n");
        gaia::system::initialize(g_configuration_file_name, nullptr);

        printf("-->Initializeing storage\n");
        init_storage();
        printf("-->Starting Simulation\n");
        sim.run();

        gaia::system::shutdown();

        return_code = EXIT_SUCCESS;
    }
    catch (std::exception& e)
    {
        printf("Simulation caught an unhandled exception: %s\n", e.what());
        sim.stop();
    }

    if (g_object_log_file != nullptr)
    {
        fclose(g_object_log_file);
        g_object_log_file = nullptr;
    }

    return return_code;
}
