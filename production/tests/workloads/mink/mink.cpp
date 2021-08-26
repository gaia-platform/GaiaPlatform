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

#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"

#include "gaia_mink.h"

using namespace std;

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::direct_access;
using namespace gaia::mink;
using namespace gaia::rules;

typedef std::chrono::steady_clock my_clock;
typedef my_clock::time_point my_time_point;
typedef std::chrono::microseconds microseconds;
typedef std::chrono::duration<double, std::micro> my_duration_in_microseconds;

const char* g_gaia_ruleset = "mink_ruleset";
const char* g_configuration_file_name = "mink.conf";

const char c_sensor_a[] = "Temp A";
const char c_sensor_b[] = "Temp B";
const char c_sensor_c[] = "Temp C";
const char c_actuator_a[] = "Fan A";
const char c_actuator_b[] = "Fan B";
const char c_actuator_c[] = "Fan C";

const char c_chicken[] = "chicken";
constexpr float c_chicken_min = 99.0;
constexpr float c_chicken_max = 102.0;

const char c_puppy[] = "puppy";
constexpr float c_puppy_min = 85.0;
constexpr float c_puppy_max = 100.0;

const double c_milliseconds_in_second = 1000.0;
const double c_microseconds_in_second = 1000000.0;

atomic<bool> g_in_simulation{false};
atomic<int> g_timestamp{0};
bool g_has_intermediate_state_output = false;

atomic<int> g_rule_1_tracker{0};
atomic<int> g_rule_2_tracker{0};
atomic<int> g_rule_3_tracker{0};
atomic<int> g_rule_4_tracker{0};

const int c_default_sleep_time_in_seconds_after_stop = 6;
const int c_default_sim_with_wait_pause_in_microseconds = 1000;
const int c_processing_pause_in_microseconds = 100;

double g_explicit_wait_time_in_microseconds = 0.0;
double g_total_wait_time_in_microseconds = 0.0;
double g_total_print_time_in_microseconds = 0.0;
double g_t_pause_total_in_microseconds = 0.0;

double g_total_start_transaction_duration_in_microseconds = 0.0;
double g_total_inside_transaction_duration_in_microseconds = 0.0;
double g_total_end_transaction_duration_in_microseconds = 0.0;
double g_update_row_duration_in_microseconds = 0.0;

long g_t_pause_requested_in_microseconds = 0;
int g_sleep_time_in_seconds_after_stop;
int g_sim_with_wait_pause_in_microseconds = c_default_sim_with_wait_pause_in_microseconds;
bool g_emit_after_pause = false;

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

// Used to get a more accurate measurement using the toggle on/off (o) command.
bool g_is_measured_duration_timer_on;
bool g_have_measurement;
my_time_point g_measured_duration_start_mark;
my_duration_in_microseconds g_measured_duration_in_microseconds;

void add_fan_control_rule();

void my_sleep_for(long parse_for_microsecond)
{
    my_time_point start_mark = my_clock::now();
    while (chrono::duration_cast<chrono::microseconds>(my_clock::now() - start_mark).count() < parse_for_microsecond)
    {
        ;
    }
}

gaia_id_t insert_incubator(const char* name, float min_temp, float max_temp)
{
    incubator_writer w;
    w.name = name;
    w.is_on = false;
    w.min_temp = min_temp;
    w.max_temp = max_temp;
    return w.insert_row();
}

void restore_sensor(sensor_t& sensor, float min_temp)
{
    sensor_writer w = sensor.writer();
    w.timestamp = 0;
    w.value = min_temp;
    w.update_row();
}

void restore_actuator(actuator_t& actuator)
{
    actuator_writer w = actuator.writer();
    w.timestamp = 0;
    w.value = 0.0;
    w.update_row();
}

void restore_incubator(incubator_t& incubator, float min_temp, float max_temp)
{
    incubator_writer w = incubator.writer();
    w.is_on = false;
    w.min_temp = min_temp;
    w.max_temp = max_temp;
    w.update_row();

    for (auto& sensor : incubator.sensors())
    {
        restore_sensor(sensor, min_temp);
    }

    for (auto& actuator : incubator.actuators())
    {
        restore_actuator(actuator);
    }
}

void restore_default_values()
{
    for (auto& incubator : incubator_t::list())
    {
        float min_temp, max_temp;
        if (strcmp(incubator.name(), c_chicken) == 0)
        {
            min_temp = c_chicken_min;
            max_temp = c_chicken_max;
        }
        else
        {
            min_temp = c_puppy_min;
            max_temp = c_puppy_max;
        }
        restore_incubator(incubator, min_temp, max_temp);
    }
}

void init_storage()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);

    // If we already have inserted an incubator then our storage has already been
    // initialized.  Re-initialize the database to default values.
    if (incubator_t::list().size())
    {
        restore_default_values();
        tx.commit();
        return;
    }

    // Chicken Incubator: 2 sensors, 1 fan
    auto incubator = incubator_t::get(
        insert_incubator(c_chicken, c_chicken_min, c_chicken_max));
    incubator.sensors().insert(
        sensor_t::insert_row(c_sensor_a, 0, c_chicken_min));
    incubator.sensors().insert(
        sensor_t::insert_row(c_sensor_c, 0, c_chicken_min));
    incubator.actuators().insert(actuator_t::insert_row(c_actuator_a, 0, 0.0));

    // Puppy Incubator: 1 sensor, 2 fans
    incubator = incubator_t::get(insert_incubator(c_puppy, c_puppy_min, c_puppy_max));
    incubator.sensors().insert(sensor_t::insert_row(c_sensor_b, 0, c_puppy_min));
    incubator.actuators().insert(actuator_t::insert_row(c_actuator_b, 0, 0.0));
    incubator.actuators().insert(actuator_t::insert_row(c_actuator_c, 0, 0.0));

    tx.commit();
}

void dump_db()
{
    begin_transaction();
    printf("\n");
    for (const gaia::mink::incubator_t& i : incubator_t::list())
    {
        printf("-----------------------------------------\n");
        printf("%-8s|power: %-3s|min: %5.1lf|max: %5.1lf\n", i.name(), i.is_on() ? "ON" : "OFF", i.min_temp(), i.max_temp());
        printf("-----------------------------------------\n");
        for (const auto& s : i.sensors())
        {
            printf("\t|%-10s|%10ld|%10.2lf\n", s.name(), s.timestamp(), s.value());
        }
        printf("\t---------------------------------\n");
        for (const auto& a : i.actuators())
        {
            printf("\t|%-10s|%10ld|%10.1lf\n", a.name(), a.timestamp(), a.value());
        }
        printf("\n");
        printf("\n");
    }
    commit_transaction();
}

void dump_db_json()
{
    begin_transaction();
    printf("{\n");
    bool is_first = true;
    for (const gaia::mink::incubator_t& i : incubator_t::list())
    {
        if (is_first)
        {
            is_first = false;
        }
        else
        {
            printf(",\n");
        }
        printf("  \"%s\" : {\n", i.name());
        // printf("    \"name\" : \"%s\",\n", i.name());
        printf("    \"power\" : \"%s\",\n", i.is_on() ? "ON" : "OFF");
        printf("    \"min_temp\" : %5.1lf,\n", i.min_temp());
        printf("    \"max_temp\" : %5.1lf,\n", i.max_temp());

        printf("    \"sensors\" : [\n");
        bool is_first_object = true;
        for (const auto& s : i.sensors())
        {
            if (is_first_object)
            {
                is_first_object = false;
            }
            else
            {
                printf(",\n");
            }
            printf("        {\n");
            printf("            \"name\" : \"%s\",\n", s.name());
            printf("            \"timestamp\" : %lu,\n", s.timestamp());
            printf("            \"value\" : %1.2lf\n", s.value());
            printf("        }");
        }
        printf("\n    ],\n");

        printf("    \"actuators\" : [\n");
        is_first_object = true;
        for (const auto& a : i.actuators())
        {
            if (is_first_object)
            {
                is_first_object = false;
            }
            else
            {
                printf(",\n");
            }
            printf("        {\n");
            printf("            \"name\" : \"%s\",\n", a.name());
            printf("            \"timestamp\" : %lu,\n", a.timestamp());
            printf("            \"value\" : %1.2lf\n", a.value());
            printf("        }");
        }
        printf("\n    ]\n");
        printf("  }");
    }
    printf("\n}\n");
    commit_transaction();
}

void toggle_measurement(bool is_live_user)
{
    if (!g_is_measured_duration_timer_on)
    {
        if (is_live_user)
        {
            printf("Measurement toggled on.");
        }
        g_is_measured_duration_timer_on = true;
        g_measured_duration_start_mark = my_clock::now();
    }
    else
    {
        my_time_point measured_duration_end_mark = my_clock::now();
        g_is_measured_duration_timer_on = false;
        g_have_measurement = true;
        if (is_live_user)
        {
            printf("Measurement toggled off.");
        }
        g_measured_duration_in_microseconds = measured_duration_end_mark - g_measured_duration_start_mark;
    }
}

float calc_new_temp(float curr_temp, float fan_speed)
{
    const int c_fan_speed_step = 500;
    const float c_temp_cool_step = 0.025;
    const float c_temp_heat_step = 0.1;

    // Simulation assumes we are in a warm environment so if no fans are on then
    // increase the temparature.
    if (fan_speed == 0)
    {
        return curr_temp + c_temp_heat_step;
    }

    // If the fans are <= 500 rpm then cool down slightly.
    int multiplier = static_cast<int>((fan_speed / c_fan_speed_step) - 1);
    if (multiplier <= 0)
    {
        return curr_temp - c_temp_cool_step;
    }

    // Otherwise cool down in proportion to the fan rpm.
    return curr_temp - (static_cast<float>(multiplier) * c_temp_cool_step);
}

void set_power(bool is_on)
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    for (auto i : incubator_t::list())
    {
        auto w = i.writer();
        w.is_on = is_on;
        w.update_row();
    }
    tx.commit();
}

void simulation_step()
{
    my_time_point start_transaction_start_mark = my_clock::now();
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    my_time_point inside_transaction_start_mark = my_clock::now();

    float new_temp = 0.0;
    float fan_a = 0.0;
    float fan_b = 0.0;
    float fan_c = 0.0;

    for (const auto& a : actuator_t::list())
    {
        if (strcmp(a.name(), c_actuator_a) == 0)
        {
            fan_a = a.value();
        }
        else if (strcmp(a.name(), c_actuator_b) == 0)
        {
            fan_b = a.value();
        }
        else if (strcmp(a.name(), c_actuator_c) == 0)
        {
            fan_c = a.value();
        }
    }

    for (auto s : sensor_t::list())
    {
        sensor_writer w = s.writer();
        if (strcmp(s.name(), c_sensor_a) == 0)
        {
            new_temp = calc_new_temp(s.value(), fan_a);
            w.value = new_temp;
            w.timestamp = g_timestamp;
        }
        else if (strcmp(s.name(), c_sensor_b) == 0)
        {
            new_temp = calc_new_temp(s.value(), fan_b);
            new_temp = calc_new_temp(new_temp, fan_c);
            w.value = new_temp;
            w.timestamp = g_timestamp;
        }
        else if (strcmp(s.name(), c_sensor_c) == 0)
        {
            new_temp = calc_new_temp(s.value(), fan_a);
            w.value = new_temp;
            w.timestamp = g_timestamp;
        }
        my_time_point update_start_mark = my_clock::now();
        w.update_row();
        my_time_point update_end_mark = my_clock::now();
        my_duration_in_microseconds update_duration = update_end_mark - update_start_mark;
        g_update_row_duration_in_microseconds += update_duration.count();
    }
    my_time_point inside_transaction_end_mark = my_clock::now();
    tx.commit();
    my_time_point commit_transaction_end_mark = my_clock::now();

    my_duration_in_microseconds start_transaction_duration = inside_transaction_start_mark - start_transaction_start_mark;
    my_duration_in_microseconds inside_transaction_duration = inside_transaction_end_mark - inside_transaction_start_mark;
    my_duration_in_microseconds end_transaction_duration = commit_transaction_end_mark - inside_transaction_end_mark;
    g_total_start_transaction_duration_in_microseconds += start_transaction_duration.count();
    g_total_inside_transaction_duration_in_microseconds += inside_transaction_duration.count();
    g_total_end_transaction_duration_in_microseconds += end_transaction_duration.count();
}

int wait_for_processing_to_complete(bool is_explicit_pause, int rule_1_sample_base, int rule_2_sample_base, int rule_3_sample_base, int rule_4_sample_base)
{
    my_time_point end_sleep_start_mark = my_clock::now();

    bool have_no_deltas = false;
    int no_deltas_count = 0;
    const int maximum_no_delta_attempts = 3000 / c_processing_pause_in_microseconds;
    const int no_delta_count_before_break = 300 / c_processing_pause_in_microseconds;
    int current_no_delta_attempt = 0;

    int timestamp = g_timestamp;

    char update_trace_buffer[c_rules_firing_update_buffer_length];
    int space_left = sizeof(update_trace_buffer);
    char* start_pointer = update_trace_buffer;

    int amount_written = 0;
    if (g_debug_log_file != nullptr)
    {
        int new_amount_written = snprintf(start_pointer, space_left, "S:%d(%d,%d,%d,%d).\n", timestamp, rule_1_sample_base, rule_2_sample_base, rule_3_sample_base, rule_4_sample_base);
        space_left -= new_amount_written;
        start_pointer += new_amount_written;
        amount_written += new_amount_written;
    }

    for (current_no_delta_attempt = 0;
         current_no_delta_attempt < maximum_no_delta_attempts;
         current_no_delta_attempt++)
    {
        my_sleep_for(c_processing_pause_in_microseconds);

        int rule_1_current_sample = g_rule_1_tracker;
        int rule_2_current_sample = g_rule_2_tracker;
        int rule_3_current_sample = g_rule_3_tracker;
        int rule_4_current_sample = g_rule_4_tracker;

        int delta_rule_1 = rule_1_current_sample - rule_1_sample_base;
        int delta_rule_2 = rule_2_current_sample - rule_2_sample_base;
        int delta_rule_3 = rule_3_current_sample - rule_3_sample_base;
        int delta_rule_4 = rule_4_current_sample - rule_4_sample_base;
        int delta_u = delta_rule_1 + delta_rule_2 + delta_rule_3 + delta_rule_4;

        if (g_debug_log_file != nullptr)
        {
            int new_amount_written = snprintf(start_pointer, space_left, "U:%d(%d,%d,%d,%d).", delta_u, delta_rule_1, delta_rule_2, delta_rule_3, delta_rule_4);
            space_left -= new_amount_written;
            start_pointer += new_amount_written;
            amount_written += new_amount_written;
        }

        if (delta_u == 0)
        {
            if (have_no_deltas)
            {
                no_deltas_count++;
            }
            else
            {
                no_deltas_count = 1;
                have_no_deltas = true;
            }
        }
        else
        {
            have_no_deltas = false;
            no_deltas_count = 0;
        }
        if (have_no_deltas && no_deltas_count >= no_delta_count_before_break)
        {
            break;
        }

        rule_1_sample_base = rule_1_current_sample;
        rule_2_sample_base = rule_2_current_sample;
        rule_3_sample_base = rule_3_current_sample;
        rule_4_sample_base = rule_4_current_sample;
    }
    my_time_point end_sleep_end_mark = my_clock::now();
    my_duration_in_microseconds ms_double = end_sleep_end_mark - end_sleep_start_mark;

    if (current_no_delta_attempt >= maximum_no_delta_attempts)
    {
        printf("...TIMEOUT: %.6f:%s\n", ms_double.count(), update_trace_buffer);
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
        int new_amount_written = snprintf(start_pointer, space_left, "\nE:%d(%d,%d,%d,%d).\n", timestamp, rule_1_sample_base, rule_2_sample_base, rule_3_sample_base, rule_4_sample_base);
        amount_written += new_amount_written;
        fwrite(update_trace_buffer, 1, amount_written, g_debug_log_file);
    }
    return current_no_delta_attempt;
}

void wait_for_processing_to_complete()
{
    int rule_1_sample_base = g_rule_1_tracker;
    int rule_2_sample_base = g_rule_2_tracker;
    int rule_3_sample_base = g_rule_3_tracker;
    int rule_4_sample_base = g_rule_4_tracker;

    wait_for_processing_to_complete(true, rule_1_sample_base, rule_2_sample_base, rule_3_sample_base, rule_4_sample_base);
}

void step()
{
    g_timestamp++;
    simulation_step();
}

void emit_state()
{
    my_time_point print_start_mark = my_clock::now();
    if (g_has_intermediate_state_output)
    {
        printf(",\n");
    }
    else
    {
        g_has_intermediate_state_output = true;
        printf("[\n");
    }
    dump_db_json();

    my_time_point print_end_mark = my_clock::now();
    my_duration_in_microseconds ms_print = print_end_mark - print_start_mark;
    g_total_print_time_in_microseconds += ms_print.count();
}

void step_and_emit_state(bool emit_text)
{
    int rule_1_sample_base = g_rule_1_tracker;
    int rule_2_sample_base = g_rule_2_tracker;
    int rule_3_sample_base = g_rule_3_tracker;
    int rule_4_sample_base = g_rule_4_tracker;

    step();

    wait_for_processing_to_complete(false, rule_1_sample_base, rule_2_sample_base, rule_3_sample_base, rule_4_sample_base);

    if (emit_text)
    {
        emit_state();
    }
}

void simulation()
{
    time_t start, cur;
    time(&start);
    begin_session();
    set_power(true);

    while (g_in_simulation)
    {
        sleep(1);
        time(&cur);
        g_timestamp = difftime(cur, start);
        simulation_step();
    }

    set_power(false);
    end_session();
}

void list_rules()
{
    subscription_list_t subs;
    const char* subscription_format = "%5s|%-20s|%-12s|%5s|%-10s|%5s\n";
    list_subscribed_rules(nullptr, nullptr, nullptr, nullptr, subs);
    printf("Number of rules for incubator: %ld\n", subs.size());
    if (subs.size() > 0)
    {
        printf("\n");
        printf(subscription_format, "line", "ruleset", "rule", "type", "event", "field");
        printf("--------------------------------------------------------------\n");
    }
    map<event_type_t, const char*> event_names;
    event_names[event_type_t::row_update] = "row update";
    event_names[event_type_t::row_insert] = "row insert";
    for (auto& s : subs)
    {
        printf(subscription_format, to_string(s->line_number).c_str(), s->ruleset_name, s->rule_name, to_string(s->gaia_type).c_str(), event_names[s->event_type], to_string(s->field).c_str());
    }
    printf("\n");
}

void add_fan_control_rule()
{
    try
    {
        subscribe_ruleset(g_gaia_ruleset);
    }
    catch (const duplicate_rule&)
    {
        printf("The ruleset has already been added.\n");
    }
}

void usage(const char* command)
{
    printf("Usage: %s [sim|show|showj|help]\n", command);
    printf(" sim: run the incubator simulation.\n");
    printf(" show: dump the tables in storage.\n");
    printf(" showj: dump the tables in storage as a json object.\n");
    printf(" help: print this message.\n");
}

class simulation_t
{
public:
    // Main menu commands.
    static constexpr char c_cmd_begin_sim = 'b';
    static constexpr char c_cmd_end_sim = 'e';
    static constexpr char c_cmd_step_sim = 's';
    static constexpr char c_cmd_step_sim_with_wait = 't';
    static constexpr char c_cmd_step_sim_with_wait_set = 'T';
    static constexpr char c_cmd_step_sim_with_wait_emit = 'U';
    static constexpr char c_cmd_step_and_emit_state_sim = 'z';
    static constexpr char c_cmd_step_and_emit_state_sim_ex = 'Z';
    static constexpr char c_cmd_toggle_debug_mode = 'D';
    static constexpr char c_cmd_list_rules = 'l';
    static constexpr char c_cmd_disable_rules = 'd';
    static constexpr char c_cmd_reenable_rules = 'r';
    static constexpr char c_cmd_print_state = 'p';
    static constexpr char c_cmd_print_state_json = 'j';
    static constexpr char c_cmd_manage_incubators = 'm';
    static constexpr char c_cmd_on_off = 'o';
    static constexpr char c_cmd_wait = 'w';
    static constexpr char c_cmd_waitx = 'x';
    static constexpr char c_cmd_comment = '#';
    static constexpr char c_cmd_quit = 'q';

    // Choose incubator commands.
    static constexpr char c_cmd_choose_chickens = 'c';
    static constexpr char c_cmd_choose_puppies = 'p';
    static constexpr char c_cmd_back = 'b';

    // Change incubator commands.
    const char* c_cmd_on = "on";
    const char* c_cmd_off = "off";
    const char* c_cmd_min = "min";
    const char* c_cmd_max = "max";
    static constexpr const char c_cmd_main = 'm';

    // Invalid input.
    const char* c_wrong_input = "Wrong input.";

    int last_known_timestamp = 0;

    void stop()
    {
        last_known_timestamp = g_timestamp;
        if (g_in_simulation)
        {
            g_in_simulation = false;
            m_simulation_thread[0].join();
            printf("Simulation stopped...\n");
        }
        if (g_debug_log_file != nullptr)
        {
            fclose(g_debug_log_file);
            g_debug_log_file = nullptr;
        }
    }

    bool handle_main(bool is_debug, bool is_live_user)
    {
        if (is_live_user)
        {
            printf("\n");
            if (!is_debug)
            {
                if (!g_in_simulation)
                {
                    printf("(%c) | begin simulation\n", c_cmd_begin_sim);
                }
                else
                {
                    printf("(%c) | end simulation \n", c_cmd_end_sim);
                }
            }
            else
            {
                printf("(%c) | step simulation\n", c_cmd_step_sim);
                printf("(%c) | step simulation with 1ns pause after\n", c_cmd_step_sim_with_wait);
                printf("(%c) | set pause for ^^^\n", c_cmd_step_sim_with_wait_set);
                printf("(%c) | set emit for ^^^\n", c_cmd_step_sim_with_wait_emit);

                printf("(%c) | step simulation and emit state\n", c_cmd_step_and_emit_state_sim);
                printf("(%c) | step simulation and emit state EX\n", c_cmd_step_and_emit_state_sim_ex);
            }
            printf("(%c) | toggle debug\n", c_cmd_toggle_debug_mode);

            printf("(%c) | list rules\n", c_cmd_list_rules);
            printf("(%c) | disable rules\n", c_cmd_disable_rules);
            printf("(%c) | toggle measurement on and off\n", c_cmd_on_off);
            printf("(%c) | re-enable rules\n", c_cmd_reenable_rules);
            printf("(%c) | print current state as raw\n", c_cmd_print_state);
            printf("(%c) | print current state as json\n", c_cmd_print_state_json);
            printf("(%c) | manage incubators\n", c_cmd_manage_incubators);
            printf("(%c) | comment line\n", c_cmd_comment);
            printf("(%c) | wait for specified milliseconds\n", c_cmd_wait);
            printf("(%c) | waitx\n", c_cmd_waitx);
            printf("(%c) | quit\n\n", c_cmd_quit);
            printf("main> ");
        }

        if (!read_input())
        {
            // Stop the simulation as well.
            return false;
        }

        if (m_input.size() == 1)
        {
            switch (m_input[0])
            {
            case c_cmd_begin_sim:
                g_in_simulation = true;
                m_simulation_thread[0] = thread(simulation);
                printf("Simulation started...\n");
                break;
            case c_cmd_end_sim:
                stop();
                break;
            case c_cmd_step_sim:
                step();
                break;
            case c_cmd_step_sim_with_wait_set:
                g_sim_with_wait_pause_in_microseconds = 1;
                break;
            case c_cmd_step_sim_with_wait_emit:
                g_emit_after_pause = true;
                break;
            case c_cmd_step_sim_with_wait:
                step();
                t_pause();
                break;
            case c_cmd_step_and_emit_state_sim:
                step_and_emit_state(true);
                break;
            case c_cmd_step_and_emit_state_sim_ex:
                step_and_emit_state(false);
                break;
            case c_cmd_toggle_debug_mode:
                if (g_debug_log_file == nullptr)
                {
                    g_debug_log_file = fopen("test-results/debug.log", "w");
                }
                else
                {
                    fclose(g_debug_log_file);
                }
                break;
            case c_cmd_list_rules:
                list_rules();
                break;
            case c_cmd_reenable_rules:
                add_fan_control_rule();
                list_rules();
                break;
            case c_cmd_disable_rules:
                unsubscribe_rules();
                list_rules();
                break;
            case c_cmd_manage_incubators:
                m_current_menu = menu_t::incubators;
                break;
            case c_cmd_print_state:
                dump_db();
                break;
            case c_cmd_print_state_json:
                dump_db_json();
                break;
            case c_cmd_on_off:
                toggle_measurement(is_live_user);
                break;
            case c_cmd_quit:
                if (g_in_simulation)
                {
                    printf("Stopping simulation...\n");
                    g_in_simulation = false;
                    m_simulation_thread[0].join();
                    printf("Simulation stopped...\n");
                }
                printf("Exiting...\n");
                return false;
                break;
            case c_cmd_wait:
                break;
            case c_cmd_waitx:
                wait_for_processing_to_complete();
                break;
            default:
                printf("%s\n", c_wrong_input);
                break;
            }
        }
        else if (m_input.size() > 1 && (m_input[0] == c_cmd_step_and_emit_state_sim || m_input[0] == c_cmd_step_and_emit_state_sim_ex || m_input[0] == c_cmd_step_sim || m_input[0] == c_cmd_step_sim_with_wait || m_input[0] == c_cmd_step_sim_with_wait_set || m_input[0] == c_cmd_wait || m_input[0] == c_cmd_comment))
        {
            if (m_input[0] == c_cmd_step_sim_with_wait_set)
            {
                char* input = m_input.data();
                int new_number = atoi(input + 1);
                if (new_number > 0)
                {
                    g_sim_with_wait_pause_in_microseconds = new_number;
                }
            }
            else if (m_input[0] == c_cmd_wait)
            {
                handle_wait();
            }
            else if (m_input[0] == c_cmd_comment)
            {
                ;
            }
            else
            {
                handle_multiple_steps();
            }
        }
        return true;
    }

    void my_sleep_for(long parse_for_microsecond)
    {
        my_time_point start_mark = my_clock::now();
        while (chrono::duration_cast<chrono::microseconds>(my_clock::now() - start_mark).count() < parse_for_microsecond)
        {
            ;
        }
    }

    void t_pause()
    {
        my_time_point pause_start_mark = my_clock::now();
        my_sleep_for(g_sim_with_wait_pause_in_microseconds);
        my_time_point pause_end_mark = my_clock::now();
        my_duration_in_microseconds printx = pause_end_mark - pause_start_mark;
        g_t_pause_total_in_microseconds += printx.count();
        g_t_pause_requested_in_microseconds += g_sim_with_wait_pause_in_microseconds;
    }

    void handle_multiple_steps()
    {
        bool is_x = m_input[0] == c_cmd_step_and_emit_state_sim_ex;
        bool is_step = m_input[0] == c_cmd_step_sim || m_input[0] == c_cmd_step_sim_with_wait;
        bool is_step_with_pause = m_input[0] == c_cmd_step_sim_with_wait;
        int limit = stoi(m_input.substr(1, m_input.size() - 1));
        for (int i = 0; i < limit; i++)
        {
            if (is_step)
            {
                step();
                if (is_step_with_pause)
                {
                    t_pause();
                    if (g_emit_after_pause)
                    {
                        emit_state();
                    }
                }
            }
            else
            {
                step_and_emit_state(!is_x);
            }
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

    void get_incubator(const char* name)
    {
        begin_transaction();
        for (const auto& incubator : incubator_t::list())
        {
            if (strcmp(incubator.name(), name) == 0)
            {
                m_current_incubator = incubator;
                break;
            }
        }
        commit_transaction();
        m_current_incubator_name = name;
    }

    bool handle_incubators(bool is_live_user)
    {
        if (is_live_user)
        {
            printf("\n");
            printf("(%c) | select chicken incubator\n", c_cmd_choose_chickens);
            printf("(%c) | select puppy incubator\n", c_cmd_choose_puppies);
            printf("(%c) | go back\n\n", c_cmd_back);
            printf("manage incubators> ");
        }
        if (!read_input())
        {
            return false;
        }
        if (m_input.size() == 1)
        {
            switch (m_input[0])
            {
            case c_cmd_choose_chickens:
                get_incubator(c_chicken);
                m_current_menu = menu_t::settings;
                break;
            case c_cmd_choose_puppies:
                get_incubator(c_puppy);
                m_current_menu = menu_t::settings;
                break;
            case c_cmd_back:
                m_current_menu = menu_t::main;
                break;
            default:
                printf("%s\n", c_wrong_input);
                break;
            }
        }

        return true;
    }

    bool handle_incubator_settings(bool is_live_user)
    {
        if (is_live_user)
        {
            printf("\n");
            printf("(%s)    | turn power on\n", c_cmd_on);
            printf("(%s)   | turn power off\n", c_cmd_off);
            printf("(%s #) | set minimum\n", c_cmd_min);
            printf("(%s #) | set maximum\n", c_cmd_max);
            printf("(%c)     | go back\n", c_cmd_back);
            printf("(%c)     | main menu\n\n", c_cmd_main);
            printf("%s> ", m_current_incubator_name);
        }

        if (!read_input())
        {
            return false;
        }

        if (m_input.size() == 1)
        {
            switch (m_input[0])
            {
            case c_cmd_back:
                m_current_menu = menu_t::incubators;
                break;
            case c_cmd_main:
                m_current_menu = menu_t::main;
                break;
            default:
                printf("%s\n", c_wrong_input);
                break;
            }
            return true;
        }

        if (m_input.size() == strlen(c_cmd_on))
        {
            if (0 == m_input.compare(c_cmd_on))
            {
                adjust_power(true);
                if (is_live_user)
                {
                    dump_db();
                }
            }
            else
            {
                printf("%s\n", c_wrong_input);
            }
            return true;
        }

        if (m_input.size() == strlen(c_cmd_off))
        {
            if (0 == m_input.compare(c_cmd_off))
            {
                adjust_power(false);
                if (is_live_user)
                {
                    dump_db();
                }
            }
            else
            {
                printf("%s\n", c_wrong_input);
            }
            return true;
        }

        bool change_min = false;
        if (0 == m_input.compare(0, 3, c_cmd_min))
        {
            change_min = true;
        }
        else if (0 == m_input.compare(0, 3, c_cmd_max))
        {
            change_min = false;
        }
        else
        {
            printf("%s\n", c_wrong_input);
            return true;
        }

        float set_point = 0;
        try
        {
            set_point = stof(m_input.substr(3, m_input.size() - 1));
        }
        catch (invalid_argument& ex)
        {
            printf("Invalid temperature setting.\n");
            return true;
        }

        if (adjust_range(change_min, set_point))
        {
            dump_db();
        }

        return true;
    }

    void adjust_power(bool turn_on)
    {
        begin_transaction();
        {
            incubator_writer w = m_current_incubator.writer();
            w.is_on = turn_on;
            w.update_row();
        }
        commit_transaction();
    }

    bool adjust_range(bool change_min, float set_point)
    {
        bool changed = false;
        begin_transaction();
        {
            incubator_writer w = m_current_incubator.writer();
            if (change_min)
            {
                w.min_temp = set_point;
            }
            else
            {
                w.max_temp = set_point;
            }
            if (w.min_temp >= w.max_temp)
            {
                printf("Max temp must be greater than min temp.\n");
            }
            else
            {
                w.update_row();
                changed = true;
            }
        }
        commit_transaction();
        return changed;
    }

    int run(bool is_debug, bool is_live_user)
    {
        bool has_input = true;
        while (has_input)
        {
            switch (m_current_menu)
            {
            case menu_t::main:
                has_input = handle_main(is_debug, is_live_user);
                break;
            case menu_t::incubators:
                has_input = handle_incubators(is_live_user);
                break;
            case menu_t::settings:
                has_input = handle_incubator_settings(is_live_user);
                break;
            default:
                // do nothing
                break;
            }
        }
        if (g_has_intermediate_state_output)
        {
            printf("]\n");
        }
        stop();
        if (!is_live_user)
        {
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
            const int c_pause_buffer_size = 100;
            char pause_buffer[c_pause_buffer_size];
            pause_buffer[0] = '\0';
            if (g_t_pause_requested_in_microseconds > 0)
            {
                snprintf(pause_buffer, sizeof(pause_buffer), ", \"requested_t_pause_in_microseconds\" : %d", g_sim_with_wait_pause_in_microseconds);
            }

            printf("{ \"stop_pause_in_sec\" : %.9f, "
                   "\"iterations\" : %d, "
                   "\"start_transaction_in_sec\" : %.9f, "
                   "\"inside_transaction_in_sec\" : %.9f, "
                   "\"end_transaction_in_sec\" : %.9f, "
                   "\"update_row_in_sec\" : %.9f, "
                   "\"total_wait_in_sec\" : %.9f, "
                   "\"explicit_wait_in_sec\" : %.9f, "
                   "\"t_pause_in_sec\" : %.9f, "
                   "\"t_requested_in_sec\" : %.9f, "
                   "\"total_print_in_sec\" : %.9f"
                   "%s%s }\n",
                   ms_double.count() / c_microseconds_in_second, last_known_timestamp, g_total_start_transaction_duration_in_microseconds / c_microseconds_in_second, g_total_inside_transaction_duration_in_microseconds / c_microseconds_in_second, g_total_end_transaction_duration_in_microseconds / c_microseconds_in_second, g_update_row_duration_in_microseconds / c_microseconds_in_second, g_total_wait_time_in_microseconds / c_microseconds_in_second, g_explicit_wait_time_in_microseconds / c_microseconds_in_second, g_t_pause_total_in_microseconds / c_microseconds_in_second, static_cast<double>(g_t_pause_requested_in_microseconds) / c_microseconds_in_second, g_total_print_time_in_microseconds / c_microseconds_in_second, measured_buffer, pause_buffer);
        }
        return EXIT_SUCCESS;
    }

private:
    enum menu_t
    {
        main,
        incubators,
        settings
    };
    string m_input;
    incubator_t m_current_incubator;
    const char* m_current_incubator_name;
    thread m_simulation_thread[1];
    menu_t m_current_menu = menu_t::main;
};

void measure_time_slices()
{
    const int slices_to_measure = 500;

    //std::this_thread::sleep_for(microseconds(c_processing_pause_in_microseconds));
    my_time_point end_sleep_start_mark = my_clock::now();
    for (int i = 0; i < slices_to_measure; i++)
    {
        std::this_thread::sleep_for(microseconds(c_processing_pause_in_microseconds));
        // my_sleep_for(1);
    }
    my_time_point end_sleep_end_mark = my_clock::now();
    my_duration_in_microseconds ms_double = end_sleep_end_mark - end_sleep_start_mark;
    printf("%.3f\n", ms_double.count());
}

int main(int argc, const char** argv)
{
    bool is_sim = false;
    bool is_debug = false;
    bool is_show_json = false;
    std::string server;
    const char* c_arg_sim = "sim";
    const char* c_arg_debug = "debug";
    const char* c_arg_show = "show";
    const char* c_arg_show_json = "showj";
    const char* c_arg_help = "help";

    if (argc == 2 && strncmp(argv[1], c_arg_sim, strlen(c_arg_sim)) == 0)
    {
        is_sim = true;
    }
    else if (argc >= 2 && strncmp(argv[1], c_arg_debug, strlen(c_arg_debug)) == 0)
    {
        is_sim = true;
        is_debug = true;
        if (argc == 3)
        {
            g_sleep_time_in_seconds_after_stop = atoi(argv[2]);
        }
        else
        {
            g_sleep_time_in_seconds_after_stop = c_default_sleep_time_in_seconds_after_stop;
        }
    }
    else if (argc == 2 && strncmp(argv[1], c_arg_show_json, strlen(c_arg_show_json)) == 0)
    {
        is_show_json = true;
    }
    else if (argc == 2 && strncmp(argv[1], c_arg_show, strlen(c_arg_show)) == 0)
    {
        is_show_json = false;
    }
    else if (argc == 2 && strncmp(argv[1], c_arg_help, strlen(c_arg_help)) == 0)
    {
        usage(argv[0]);
        return EXIT_SUCCESS;
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

    if (!is_sim)
    {
        gaia::system::initialize();
        gaia::rules::unsubscribe_rules();
        if (is_show_json)
        {
            dump_db_json();
        }
        else
        {
            dump_db();
        }
        return EXIT_SUCCESS;
    }

    bool is_live_user = isatty(fileno(stdin));
    if (is_live_user)
    {
        printf("Stdin is not a file or a pipe.  Activating interactive mode.\n");
    }

    if (false)
    {
        measure_time_slices();
    }
    else
    {
        simulation_t sim;
        gaia::system::initialize(g_configuration_file_name, nullptr);

        if (is_live_user)
        {
            printf("-----------------------------------------\n");
            printf("Gaia Incubator\n\n");
            printf("No chickens or puppies were harmed in the\n");
            printf("development or presentation of this demo.\n");
            printf("-----------------------------------------\n");
        }

        init_storage();
        sim.run(is_debug, is_live_user);
        gaia::system::shutdown();
    }
}
