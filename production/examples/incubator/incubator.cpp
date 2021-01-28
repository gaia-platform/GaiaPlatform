/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <unistd.h>

#include <cstring>
#include <ctime>

#include <algorithm>
#include <atomic>
#include <iostream>
#include <string>
#include <thread>

#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"

#include "gaia_incubator.h"

using namespace std;

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::direct_access;
using namespace gaia::incubator;
using namespace gaia::rules;

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

atomic<bool> g_in_simulation{false};
atomic<int> g_timestamp{0};

void add_fan_control_rule();

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

    for (auto& sensor : incubator.sensor_list())
    {
        restore_sensor(sensor, min_temp);
    }

    for (auto& actuator : incubator.actuator_list())
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
    if (incubator_t::get_first())
    {
        restore_default_values();
        tx.commit();
        return;
    }

    // Chicken Incubator: 2 sensors, 1 fan
    auto incubator = incubator_t::get(insert_incubator(c_chicken, c_chicken_min, c_chicken_max));
    incubator.sensor_list().insert(sensor_t::insert_row(c_sensor_a, 0, c_chicken_min));
    incubator.sensor_list().insert(sensor_t::insert_row(c_sensor_c, 0, c_chicken_min));
    incubator.actuator_list().insert(actuator_t::insert_row(c_actuator_a, 0, 0.0));

    // Puppy Incubator: 1 sensor, 2 fans
    incubator = incubator_t::get(insert_incubator(c_puppy, c_puppy_min, c_puppy_max));
    incubator.sensor_list().insert(sensor_t::insert_row(c_sensor_b, 0, c_puppy_min));
    incubator.actuator_list().insert(actuator_t::insert_row(c_actuator_b, 0, 0.0));
    incubator.actuator_list().insert(actuator_t::insert_row(c_actuator_c, 0, 0.0));

    tx.commit();
}

void dump_db()
{
    begin_transaction();
    printf("\n");
    for (auto i : incubator_t::list())
    {
        printf("-----------------------------------------\n");
        printf("%-8s|power: %-3s|min: %5.1lf|max: %5.1lf\n", i.name(), i.is_on() ? "ON" : "OFF", i.min_temp(), i.max_temp());
        printf("-----------------------------------------\n");
        for (const auto& s : i.sensor_list())
        {
            printf("\t|%-10s|%10ld|%10.2lf\n", s.name(), s.timestamp(), s.value());
        }
        printf("\t---------------------------------\n");
        for (const auto& a : i.actuator_list())
        {
            printf("\t|%-10s|%10ld|%10.1lf\n", a.name(), a.timestamp(), a.value());
        }
        printf("\n");
        printf("\n");
    }
    commit_transaction();
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

void simulation()
{
    time_t start, cur;
    time(&start);
    begin_session();
    set_power(true);

    while (g_in_simulation)
    {
        sleep(1);

        auto_transaction_t tx(auto_transaction_t::no_auto_begin);

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

        time(&cur);
        g_timestamp = difftime(cur, start);
        for (auto s : sensor_t::list())
        {
            sensor_writer w = s.writer();
            if (strcmp(s.name(), c_sensor_a) == 0)
            {
                new_temp = calc_new_temp(s.value(), fan_a);
                w.value = new_temp;
                w.timestamp = g_timestamp;
                w.update_row();
            }
            else if (strcmp(s.name(), c_sensor_b) == 0)
            {
                new_temp = calc_new_temp(s.value(), fan_b);
                new_temp = calc_new_temp(new_temp, fan_c);
                w.value = new_temp;
                w.timestamp = g_timestamp;
                w.update_row();
            }
            else if (strcmp(s.name(), c_sensor_c) == 0)
            {
                new_temp = calc_new_temp(s.value(), fan_a);
                w.value = new_temp;
                w.timestamp = g_timestamp;
                w.update_row();
            }
        }
        tx.commit();
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
        subscribe_ruleset("incubator_ruleset");
    }
    catch (const duplicate_rule&)
    {
        printf("The ruleset has already been added.\n");
    }
}

void usage(const char* command)
{
    printf("Usage: %s [sim|show|help]\n", command);
    printf(" sim: run the incubator simulation.\n");
    printf(" show: dump the tables in storage.\n");
    printf(" help: print this message.\n");
}

class simulation_t
{
public:
    // Main menu commands.
    static constexpr char c_cmd_begin_sim = 'b';
    static constexpr char c_cmd_end_sim = 'e';
    static constexpr char c_cmd_list_rules = 'l';
    static constexpr char c_cmd_disable_rules = 'd';
    static constexpr char c_cmd_reenable_rules = 'r';
    static constexpr char c_cmd_print_state = 'p';
    static constexpr char c_cmd_manage_incubators = 'm';
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

    void stop()
    {
        if (g_in_simulation)
        {
            g_in_simulation = false;
            m_simulation_thread[0].join();
            printf("Simulation stopped...\n");
        }
    }

    bool handle_main()
    {
        printf("\n");
        printf("(%c) | begin simulation\n", c_cmd_begin_sim);
        printf("(%c) | end simulation \n", c_cmd_end_sim);
        printf("(%c) | list rules\n", c_cmd_list_rules);
        printf("(%c) | disable rules\n", c_cmd_disable_rules);
        printf("(%c) | re-enable rules\n", c_cmd_reenable_rules);
        printf("(%c) | print current state\n", c_cmd_print_state);
        printf("(%c) | manage incubators\n", c_cmd_manage_incubators);
        printf("(%c) | quit\n\n", c_cmd_quit);
        printf("main> ");

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
                if (!g_in_simulation)
                {
                    g_in_simulation = true;
                    m_simulation_thread[0] = thread(simulation);
                    printf("Simulation started...\n");
                }
                else
                {
                    printf("Simulation is already running.\n");
                }
                break;
            case c_cmd_end_sim:
                stop();
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
            default:
                printf("%s\n", c_wrong_input);
                break;
            }
        }
        return true;
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

    bool handle_incubators()
    {
        printf("\n");
        printf("(%c) | select chicken incubator\n", c_cmd_choose_chickens);
        printf("(%c) | select puppy incubator\n", c_cmd_choose_puppies);
        printf("(%c) | go back\n\n", c_cmd_back);
        printf("manage incubators> ");
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

    bool handle_incubator_settings()
    {
        printf("\n");
        printf("(%s)    | turn power on\n", c_cmd_on);
        printf("(%s)   | turn power off\n", c_cmd_off);
        printf("(%s #) | set minimum\n", c_cmd_min);
        printf("(%s #) | set maximum\n", c_cmd_max);
        printf("(%c)     | go back\n", c_cmd_back);
        printf("(%c)     | main menu\n\n", c_cmd_main);
        printf("%s> ", m_current_incubator_name);

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
                dump_db();
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
                dump_db();
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

    int run()
    {
        bool has_input = true;
        while (has_input)
        {
            switch (m_current_menu)
            {
            case menu_t::main:
                has_input = handle_main();
                break;
            case menu_t::incubators:
                has_input = handle_incubators();
                break;
            case menu_t::settings:
                has_input = handle_incubator_settings();
                break;
            default:
                // do nothing
                break;
            }
        }
        stop();
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

int main(int argc, const char** argv)
{
    bool is_sim = false;
    std::string server;
    const char* c_arg_sim = "sim";
    const char* c_arg_show = "show";
    const char* c_arg_help = "help";

    if (argc == 2 && strncmp(argv[1], c_arg_sim, strlen(c_arg_sim)) == 0)
    {
        is_sim = true;
    }
    else if (argc == 2 && strncmp(argv[1], c_arg_show, strlen(c_arg_show)) == 0)
    {
        is_sim = false;
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
        dump_db();
        return EXIT_SUCCESS;
    }

    simulation_t sim;
    gaia::system::initialize();

    printf("-----------------------------------------\n");
    printf("Gaia Incubator\n\n");
    printf("No chickens or puppies were harmed in the\n");
    printf("development or presentation of this demo.\n");
    printf("-----------------------------------------\n");

    init_storage();
    sim.run();
    gaia::system::shutdown();
}
