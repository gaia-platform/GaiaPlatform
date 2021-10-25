/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "simulation.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::direct_access;
using namespace gaia::rules;

atomic<uint64_t> g_timestamp{1};

uint64_t get_time_millis()
{
    return g_timestamp++;
}

void simulation_t::toggle_measurement()
{
    if (!m_is_measured_duration_timer_on)
    {
        printf("Measure timer activated.\n");
        m_is_measured_duration_timer_on = true;
        m_measured_duration_start_mark = my_clock::now();
    }
    else
    {
        my_time_point measured_duration_end_mark = my_clock::now();
        m_is_measured_duration_timer_on = false;
        m_have_measurement = true;
        m_measured_duration_in_microseconds = measured_duration_end_mark - m_measured_duration_start_mark;
        printf("Measure timer deactivated.\n");
    }
}

void simulation_t::toggle_debug_mode()
{
    if (m_debug_log_file == nullptr)
    {
        printf("Debug log activated.\n");
        m_debug_log_file = fopen("test-results/debug.log", "w");
    }
    else
    {
        fclose(m_debug_log_file);
        printf("Debug log deactivated.\n");
    }
}

long simulation_t::get_processing_timeout_in_microseconds()
{
    return c_default_processing_complete_timeout_in_microseconds;
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

    my_time_point start_transaction_start_mark = my_clock::now();
    auto_transaction_t txn(auto_transaction_t::no_auto_begin);
    my_time_point inside_transaction_start_mark = my_clock::now();

    if (!m_has_database_been_initialized)
    {
        handle_initialize("");
    }

    setup_test_data(limit);

    my_time_point inside_transaction_end_mark = my_clock::now();
    txn.commit();
    my_time_point commit_transaction_end_mark = my_clock::now();

    wait_for_processing_to_complete(false, get_processing_timeout_in_microseconds());

    my_duration_in_microseconds start_transaction_duration = inside_transaction_start_mark - start_transaction_start_mark;
    my_duration_in_microseconds inside_transaction_duration = inside_transaction_end_mark - inside_transaction_start_mark;
    my_duration_in_microseconds end_transaction_duration = commit_transaction_end_mark - inside_transaction_end_mark;
    m_total_start_transaction_duration_in_microseconds += start_transaction_duration.count();
    m_total_inside_transaction_duration_in_microseconds += inside_transaction_duration.count();
    m_total_end_transaction_duration_in_microseconds += end_transaction_duration.count();

    m_number_of_test_iterations = limit;
}

void simulation_t::wait_for_processing_to_complete(bool is_explicit_pause, long timeout_in_microseconds)
{
    my_time_point end_sleep_start_mark = my_clock::now();

    const long maximum_no_delta_attempts = timeout_in_microseconds / c_processing_pause_in_microseconds;

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
        my_time_point check_start_mark = my_clock::now();
        bool have_completed = has_test_completed();
        my_time_point check_end_mark = my_clock::now();
        my_duration_in_microseconds ms_double = check_end_mark - check_start_mark;
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
        precise_sleep_for(c_processing_pause_in_microseconds);
    }
    my_time_point end_sleep_end_mark = my_clock::now();
    my_duration_in_microseconds ms_double = end_sleep_end_mark - end_sleep_start_mark;

    if (current_no_delta_attempt == maximum_no_delta_attempts)
    {
        printf("\nwait_for_processing_to_complete -- TIMEOUT!!!\n");
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

void simulation_t::precise_sleep_for(long parse_for_microsecond)
{
    my_time_point start_mark = my_clock::now();
    while (chrono::duration_cast<chrono::microseconds>(my_clock::now() - start_mark).count() < parse_for_microsecond)
    {
        // Keep waiting for time to elapse.
    }
}

void simulation_t::handle_wait()
{
    my_time_point end_sleep_start_mark = my_clock::now();

    int limit = stoi(m_command_input_line.substr(1, m_command_input_line.size() - 1));
    std::this_thread::sleep_for(
        microseconds(
            limit * (static_cast<long>(c_microseconds_in_second) / static_cast<long>(c_milliseconds_in_second))));

    my_time_point end_sleep_end_mark = my_clock::now();

    my_duration_in_microseconds ms_double = end_sleep_end_mark - end_sleep_start_mark;
    m_explicit_wait_time_in_microseconds += ms_double.count();
}

// Return false if EOF is reached.
bool simulation_t::read_input()
{
    getline(cin, m_command_input_line);
    return !cin.eof();
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
        switch (m_command_input_line[0])
        {
        case c_cmd_toggle_measurement:
            toggle_measurement();
            break;
        case c_cmd_toggle_debug_mode:
            toggle_debug_mode();
            break;
        case c_cmd_initialize:
            handle_initialize("");
            break;
        default:
            printf("Wrong input: %c\n", m_command_input_line[0]);
            break;
        }
    }
    else if (m_command_input_line.size() > 1 && (m_command_input_line[0] == c_cmd_wait || m_command_input_line[0] == c_cmd_comment || m_command_input_line[0] == c_cmd_test || m_command_input_line[0] == c_cmd_initialize))
    {
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
        else if (m_command_input_line[0] == c_cmd_test)
        {
            handle_test(m_command_input_line);
        }
    }
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

    my_time_point end_sleep_start_mark = my_clock::now();
    sleep(m_sleep_time_in_seconds_after_stop);
    my_time_point end_sleep_end_mark = my_clock::now();
    my_duration_in_microseconds ms_double = end_sleep_end_mark - end_sleep_start_mark;

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
            printf("Simulation caught an unhandled exception: %s\n", e.what());
            close_open_log_files();
        }
    }

    return return_code;
}
