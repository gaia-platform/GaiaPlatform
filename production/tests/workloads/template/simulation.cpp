/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "simulation.hpp"

#include <cxxabi.h>
#include <execinfo.h>

#include <iostream>

using namespace std;

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::direct_access;
using namespace gaia::rules;

atomic<uint64_t> g_timestamp{1};
atomic<int> g_rule_1_tracker{0};

uint64_t generate_unique_millisecond_timestamp()
{
    return g_timestamp++;
}

void simulation_t::toggle_measurement()
{
    if (!m_is_measured_duration_timer_on)
    {
        printf("Measure timer has been activated.\n");
        m_is_measured_duration_timer_on = true;
        m_measured_duration_start_mark = my_clock_t::now();
    }
    else
    {
        my_time_point_t measured_duration_end_mark = my_clock_t::now();
        m_is_measured_duration_timer_on = false;
        m_have_measurement = true;
        m_measured_duration_in_microseconds = measured_duration_end_mark - m_measured_duration_start_mark;
        printf("Measure timer has been deactivated.\n");
    }
}

void simulation_t::toggle_debug_mode()
{
    if (m_debug_log_file == nullptr)
    {
        printf("Debug log has been activated.\n");
        m_debug_log_file = fopen("test-results/debug.log", "w");
    }
    else
    {
        fclose(m_debug_log_file);
        printf("Debug log has been deactivated.\n");
    }
}

void simulation_t::toggle_pause_mode()
{
    if (m_use_transaction_wait)
    {
        printf("Wait mode has been set to atomic.\n");
        m_use_transaction_wait = false;
    }
    else
    {
        printf("Wait mode has been set to transaction.\n");
        m_use_transaction_wait = true;
    }
}

bool simulation_t::get_use_transaction_wait()
{
    return m_use_transaction_wait;
}

long simulation_t::get_processing_timeout_in_microseconds()
{
    return c_default_processing_complete_timeout_in_microseconds;
}

long simulation_t::get_processing_pause_in_microseconds()
{
    return c_processing_pause_in_microseconds;
}

void simulation_t::dump_db_json(FILE* object_log_file)
{
    begin_transaction();
    fprintf(object_log_file, "{\n");
    fprintf(object_log_file, "}\n");
    commit_transaction();
}

void simulation_t::handle_initialize(const string& input)
{
    m_has_database_been_initialized = true;
    init_storage();
}

void simulation_t::handle_test(const string& input)
{
    int limit = stoi(input.substr(1, input.size() - 1));

    if (!m_has_database_been_initialized)
    {
        handle_initialize("");
    }

    my_time_point_t start_transaction_start_mark = my_clock_t::now();
    auto_transaction_t txn(auto_transaction_t::no_auto_begin);
    my_time_point_t inside_transaction_start_mark = my_clock_t::now();

    setup_test_data(limit);

    my_time_point_t inside_transaction_end_mark = my_clock_t::now();
    txn.commit();
    my_time_point_t commit_transaction_end_mark = my_clock_t::now();

    wait_for_test_processing_to_complete(false, get_processing_timeout_in_microseconds(), get_processing_pause_in_microseconds());

    my_duration_in_microseconds_t start_transaction_duration = inside_transaction_start_mark - start_transaction_start_mark;
    my_duration_in_microseconds_t inside_transaction_duration = inside_transaction_end_mark - inside_transaction_start_mark;
    my_duration_in_microseconds_t end_transaction_duration = commit_transaction_end_mark - inside_transaction_end_mark;
    m_total_start_transaction_duration_in_microseconds += start_transaction_duration.count();
    m_total_inside_transaction_duration_in_microseconds += inside_transaction_duration.count();
    m_total_end_transaction_duration_in_microseconds += end_transaction_duration.count();

    m_number_of_test_iterations = limit;
}

void simulation_t::wait_for_test_processing_to_complete(bool is_explicit_pause, long timeout_in_microseconds, long pause_in_microseconds)
{
    my_time_point_t end_sleep_start_mark = my_clock_t::now();

    const long maximum_no_delta_attempts = timeout_in_microseconds / pause_in_microseconds;
    int timestamp = g_timestamp;

    char trace_buffer[c_rules_firing_update_buffer_length];
    char update_trace_buffer[c_rules_firing_update_buffer_length];
    unsigned long space_left = sizeof(update_trace_buffer);
    char* start_pointer = update_trace_buffer;
    int amount_written = 0;

    long current_no_delta_attempt;
    for (current_no_delta_attempt = 0;
         current_no_delta_attempt < maximum_no_delta_attempts;
         current_no_delta_attempt++)
    {
        my_time_point_t check_start_mark = my_clock_t::now();
        bool has_completed = has_test_completed();
        my_time_point_t check_end_mark = my_clock_t::now();
        my_duration_in_microseconds_t ms_double = check_end_mark - check_start_mark;
        m_check_time_in_microseconds += ms_double.count();

        if (m_debug_log_file != nullptr)
        {
            int new_amount_written = snprintf(start_pointer, space_left, "%d,", static_cast<int>(has_completed));
            space_left -= new_amount_written;
            start_pointer += new_amount_written;
            amount_written += new_amount_written;
        }
        if (has_completed)
        {
            break;
        }
        sleep_for(pause_in_microseconds);
    }
    my_time_point_t end_sleep_end_mark = my_clock_t::now();
    my_duration_in_microseconds_t ms_double = end_sleep_end_mark - end_sleep_start_mark;

    if (current_no_delta_attempt == maximum_no_delta_attempts)
    {
        printf("\nwait_for_test_processing_to_complete -- TIMEOUT!!!\n");
    }

    if (is_explicit_pause)
    {
        m_explicit_wait_time_in_microseconds += ms_double.count();
    }
    else
    {
        m_total_wait_time_in_microseconds += ms_double.count();
    }
    if (m_debug_log_file != nullptr)
    {
        int new_amount_written = snprintf(trace_buffer, sizeof(trace_buffer), "%d,%.9f,%s\n", timestamp, ms_double.count(), update_trace_buffer);
        fwrite(trace_buffer, 1, new_amount_written, m_debug_log_file);
    }
}

void simulation_t::display_usage(const char* command)
{
    printf("Usage: %s debug <input-file>\n", command);
}

void simulation_t::sleep_for(long parse_for_microsecond)
{
    my_time_point_t start_mark = my_clock_t::now();
    while (chrono::duration_cast<chrono::microseconds>(my_clock_t::now() - start_mark).count() < parse_for_microsecond)
    {
        // Keep waiting for time to elapse.
    }
}

void simulation_t::handle_wait()
{
    my_time_point_t end_sleep_start_mark = my_clock_t::now();

    int limit = stoi(m_command_input_line.substr(1, m_command_input_line.size() - 1));
    std::this_thread::sleep_for(
        std::chrono::microseconds(
            limit * (static_cast<long>(c_microseconds_in_second) / static_cast<long>(c_milliseconds_in_second))));

    my_time_point_t end_sleep_end_mark = my_clock_t::now();

    my_duration_in_microseconds_t ms_double = end_sleep_end_mark - end_sleep_start_mark;
    m_explicit_wait_time_in_microseconds += ms_double.count();
}

// Return false if EOF is reached.
bool simulation_t::read_input()
{
    getline(cin, m_command_input_line);
    return !cin.eof();
}

void simulation_t::simulation_step()
{
    my_time_point_t start_transaction_start_mark = my_clock_t::now();
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    my_time_point_t inside_transaction_start_mark = my_clock_t::now();

    my_duration_in_microseconds_t update_duration = perform_single_step();
    m_update_row_duration_in_microseconds += update_duration.count();

    my_time_point_t inside_transaction_end_mark = my_clock_t::now();
    tx.commit();
    my_time_point_t commit_transaction_end_mark = my_clock_t::now();

    my_duration_in_microseconds_t start_transaction_duration = inside_transaction_start_mark - start_transaction_start_mark;
    my_duration_in_microseconds_t inside_transaction_duration = inside_transaction_end_mark - inside_transaction_start_mark;
    my_duration_in_microseconds_t end_transaction_duration = commit_transaction_end_mark - inside_transaction_end_mark;
    m_total_start_transaction_duration_in_microseconds += start_transaction_duration.count();
    m_total_inside_transaction_duration_in_microseconds += inside_transaction_duration.count();
    m_total_end_transaction_duration_in_microseconds += end_transaction_duration.count();
}

int simulation_t::wait_for_step_processing_to_complete(bool is_explicit_pause, int initial_state_tracker, int timeout_in_microseconds)
{
    my_time_point_t end_sleep_start_mark = my_clock_t::now();

    const int maximum_no_delta_attempts = timeout_in_microseconds / c_processing_pause_in_microseconds;

    int timestamp = g_timestamp;

    char trace_buffer[c_rules_firing_update_buffer_length];
    char update_trace_buffer[c_rules_firing_update_buffer_length];
    unsigned long space_left = sizeof(update_trace_buffer);
    char* start_pointer = update_trace_buffer;
    int amount_written = 0;

    int current_no_delta_attempt;

    for (current_no_delta_attempt = 0;
         current_no_delta_attempt < maximum_no_delta_attempts;
         current_no_delta_attempt++)
    {
        my_time_point_t check_start_mark = my_clock_t::now();
        bool have_completed = has_step_completed(timestamp, initial_state_tracker);
        my_time_point_t check_end_mark = my_clock_t::now();
        my_duration_in_microseconds_t ms_double = check_end_mark - check_start_mark;
        m_check_time_in_microseconds += ms_double.count();

        if (m_debug_log_file != nullptr)
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
        sleep_for(c_processing_pause_in_microseconds);
    }
    my_time_point_t end_sleep_end_mark = my_clock_t::now();
    my_duration_in_microseconds_t ms_double = end_sleep_end_mark - end_sleep_start_mark;

    if (current_no_delta_attempt == maximum_no_delta_attempts)
    {
        printf("\nwait_for_step_processing_to_complete -- TIMEOUT!!!\n");
    }

    if (is_explicit_pause)
    {
        m_explicit_wait_time_in_microseconds += ms_double.count();
    }
    else
    {
        m_total_wait_time_in_microseconds += ms_double.count();
    }
    if (m_debug_log_file != nullptr)
    {
        int new_amount_written = snprintf(trace_buffer, sizeof(trace_buffer), "%d,%.9f,%s\n", timestamp, ms_double.count(), update_trace_buffer);
        fwrite(trace_buffer, 1, new_amount_written, m_debug_log_file);
    }
    return current_no_delta_attempt;
}

void simulation_t::handle_multiple_steps(const string& input)
{
    int limit = stoi(input.substr(1, input.size() - 1));
    for (int i = 0; i < limit; i++)
    {
        int initial_state_tracker = g_rule_1_tracker;
        g_timestamp++;
        simulation_step();
        wait_for_step_processing_to_complete(get_use_transaction_wait(), initial_state_tracker, c_normal_wait_timeout_in_microseconds);
    }

    m_number_of_test_iterations = limit;
}

bool simulation_t::handle_main()
{
    if (!read_input())
    {
        // Stop the simulation as well.
        return false;
    }

    printf("Input: %s\n", m_command_input_line.c_str());
    if (m_command_input_line.size() == 1)
    {
        printf("Processing single character input: %c\n", m_command_input_line[0]);
        switch (m_command_input_line[0])
        {
        case c_cmd_toggle_measurement:
            toggle_measurement();
            break;
        case c_cmd_toggle_pause_mode:
            toggle_pause_mode();
            break;
        case c_cmd_toggle_debug_mode:
            toggle_debug_mode();
            break;
        case c_cmd_initialize:
            handle_initialize("");
            break;
        case c_cmd_state:
            dump_db_json(m_object_log_file);
            break;
        default:
            printf("Wrong single character input: %c\n", m_command_input_line[0]);
            break;
        }
    }
    else if (m_command_input_line.size() > 1 && (m_command_input_line[0] == c_cmd_wait || m_command_input_line[0] == c_cmd_step_sim || m_command_input_line[0] == c_cmd_comment || m_command_input_line[0] == c_cmd_test || m_command_input_line[0] == c_cmd_initialize))
    {
        printf("Processing multiple character input: %s\n", m_command_input_line.c_str());
        if (m_command_input_line[0] == c_cmd_wait)
        {
            handle_wait();
        }
        else if (m_command_input_line[0] == c_cmd_comment)
        {
            ;
        }
        else if (m_command_input_line[0] == c_cmd_initialize)
        {
            handle_initialize(m_command_input_line);
        }
        else if (m_command_input_line[0] == c_cmd_step_sim)
        {
            handle_multiple_steps(m_command_input_line);
        }
        else if (m_command_input_line[0] == c_cmd_test)
        {
            handle_test(m_command_input_line);
        }
        else
        {
            printf("Wrong multiple character input: %s\n", m_command_input_line.c_str());
        }
    }
    else
    {
        printf("Wrong character input: %s\n", m_command_input_line.c_str());
    }
    printf("Command processed.\n");
    return true;
}

void simulation_t::close_open_log_files()
{
    if (m_debug_log_file != nullptr)
    {
        fclose(m_debug_log_file);
        m_debug_log_file = nullptr;
    }

    if (m_object_log_file != nullptr)
    {
        fclose(m_object_log_file);
        m_object_log_file = nullptr;
    }
}

int simulation_t::run_simulation()
{
    bool has_input = true;
    while (has_input)
    {
        has_input = handle_main();
    }

    if (m_has_intermediate_state_output)
    {
        fprintf(m_object_log_file, "]\n");
    }

    close_open_log_files();

    my_time_point_t end_sleep_start_mark = my_clock_t::now();
    sleep(m_sleep_time_in_seconds_after_stop);
    my_time_point_t end_sleep_end_mark = my_clock_t::now();
    my_duration_in_microseconds_t ms_double = end_sleep_end_mark - end_sleep_start_mark;

    const int c_measured_buffer_size = 100;
    char measured_buffer[c_measured_buffer_size];
    measured_buffer[0] = '\0';
    if (m_have_measurement)
    {
        double measured_in_secs = m_measured_duration_in_microseconds.count() / c_microseconds_in_second;
        snprintf(measured_buffer, sizeof(measured_buffer), ", \"measured_in_sec\" : %.9f", measured_in_secs);
    }

    FILE* delays_file = fopen("test-results/output.delay", "w");
    fprintf(
        delays_file,
        "{ \"stop_pause_in_sec\" : %.9f, "
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
        ms_double.count() / c_microseconds_in_second,
        m_number_of_test_iterations,
        m_total_start_transaction_duration_in_microseconds / c_microseconds_in_second,
        m_total_inside_transaction_duration_in_microseconds / c_microseconds_in_second,
        m_total_end_transaction_duration_in_microseconds / c_microseconds_in_second,
        m_update_row_duration_in_microseconds / c_microseconds_in_second,
        m_total_wait_time_in_microseconds / c_microseconds_in_second,
        m_explicit_wait_time_in_microseconds / c_microseconds_in_second,
        m_check_time_in_microseconds / c_microseconds_in_second,
        m_total_print_time_in_microseconds / c_microseconds_in_second,
        measured_buffer);
    fclose(delays_file);
    return EXIT_SUCCESS;
}

int simulation_t::handle_command_line_arguments(int argc, const char** argv)
{
    const char* c_arg_debug = "debug";
    const int c_debug_parameter_index = 2;
    if (argc >= c_debug_parameter_index && strncmp(argv[1], c_arg_debug, strlen(c_arg_debug)) == 0)
    {
        if (argc > c_debug_parameter_index)
        {
            m_sleep_time_in_seconds_after_stop = atoi(argv[c_debug_parameter_index]);
        }
        else
        {
            m_sleep_time_in_seconds_after_stop = c_default_sleep_time_in_seconds_after_stop;
        }
    }
    else
    {
        if (argc >= c_debug_parameter_index)
        {
            cout << "Wrong arguments." << endl;
        }
        display_usage(argv[0]);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void simulation_t::dump_exception_stack()
{
    const int maximum_stack_depth = 25;
    const int maximum_function_name_size = 256;

    void* address_list[maximum_stack_depth];
    size_t address_length = backtrace(address_list, sizeof(address_list) / sizeof(void*));
    backtrace_symbols_fd(address_list, address_length, STDERR_FILENO);

    char** symbol_list = backtrace_symbols(address_list, address_length);

    size_t function_name_size = maximum_function_name_size;
    auto function_name_buffer = std::unique_ptr<char[]>(new char[function_name_size]);
    char* function_name = function_name_buffer.get();

    for (size_t i = 1; i < address_length; i++)
    {
        char *begin_name = nullptr, *begin_offset = nullptr, *end_offset = nullptr;

        // find parentheses and +address offset surrounding the mangled name:
        // ./module(function+0x15c) [0x8048a6d]
        for (char* p = symbol_list[i]; *p; ++p)
        {
            if (*p == '(')
                begin_name = p;
            else if (*p == '+')
                begin_offset = p;
            else if (*p == ')' && begin_offset)
            {
                end_offset = p;
                break;
            }
        }

        if (begin_name && begin_offset && end_offset
            && begin_name < begin_offset)
        {
            *begin_name++ = '\0';
            *begin_offset++ = '\0';
            *end_offset = '\0';

            // mangled name is now in [begin_name, begin_offset) and caller
            // offset in [begin_offset, end_offset). now apply
            // __cxa_demangle():

            int status;
            char* ret = abi::__cxa_demangle(begin_name, function_name, &function_name_size, &status);
            if (status == 0)
            {
                function_name = ret; // use possibly realloc()-ed string
                printf("  %s : %s+%s\n", symbol_list[i], function_name, begin_offset);
            }
            else
            {
                // demangling failed. Output function name as a C function with
                // no arguments.
                printf("  %s : %s()+%s\n", symbol_list[i], begin_name, begin_offset);
            }
        }
        else
        {
            // couldn't parse the line? print the whole line.
            printf("  %s\n", symbol_list[i]);
        }
    }
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    free(symbol_list);
}

int simulation_t::main(int argc, const char** argv)
{
    int return_code = handle_command_line_arguments(argc, argv);
    if (return_code == EXIT_SUCCESS)
    {
        try
        {
            m_object_log_file = fopen("test-results/output.json", "w");

            gaia::system::initialize(get_configuration_file_name().c_str(), nullptr);

            run_simulation();

            gaia::system::shutdown();

            return_code = EXIT_SUCCESS;
        }
        catch (std::exception& e)
        {
            printf("std complete\n");
            printf("Simulation caught a exception: %s\n", e.what());
            close_open_log_files();
            dump_exception_stack();
        }
        catch (...)
        {
            printf("all complete\n");
            std::exception_ptr p = std::current_exception();
            printf("Simulation caught an unhandled exception: %s\n", (p ? p.__cxa_exception_type()->name() : "null"));
            close_open_log_files();
            dump_exception_stack();
        }
        printf("Processing complete\n");
    }

    return return_code;
}
