/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
/////////////////////////////////////////////

#include <chrono>
#include <cstring>
#include <ctime>

#include <algorithm>
#include <atomic>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>

#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"

#include "gaia_incubator.h"

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

std::atomic<bool> g_in_simulation{false};
std::atomic<int> g_timestamp{0};

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
    auto_transaction_t tx(auto_transaction_t::no_auto_restart);

    // If we already have inserted an incubator then our storage has already been
    // initialized.  Re-initialize the database to default values.
    if (incubator_t::list().size())
    {
        restore_default_values();
        tx.commit();
        return;
    }

    // Chicken Incubator: 2 sensors, 1 fan
    auto incubator = incubator_t::get(insert_incubator(c_chicken, c_chicken_min, c_chicken_max));
    incubator.sensors().insert(sensor_t::insert_row(c_sensor_a, 0, c_chicken_min));
    incubator.sensors().insert(sensor_t::insert_row(c_sensor_c, 0, c_chicken_min));
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
    std::cout << "\n";
    std::cout << std::fixed << std::setprecision(1);
    for (auto i : incubator_t::list())
    {
        std::cout << "-----------------------------------------\n";
        std::printf(
            "%-8s|power: %-3s|min: %5.1lf|max: %5.1lf\n",
            i.name(), i.is_on() ? "ON" : "OFF", i.min_temp(), i.max_temp());
        std::cout << "-----------------------------------------\n";
        for (const auto& s : i.sensors())
        {
            std::printf("\t|%-10s|%10ld|%10.2lf\n", s.name(), s.timestamp(), s.value());
        }
        std::cout << "\t---------------------------------\n";
        for (const auto& a : i.actuators())
        {
            std::printf("\t|%-10s|%10ld|%10.1lf\n", a.name(), a.timestamp(), a.value());
        }
        std::cout << "\n"
                  << std::endl;
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
    auto_transaction_t tx(auto_transaction_t::no_auto_restart);
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
    auto start = std::chrono::steady_clock::now();
    begin_session();
    set_power(true);

    while (g_in_simulation)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        auto_transaction_t tx(auto_transaction_t::no_auto_restart);

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

        auto current = std::chrono::steady_clock::now();
        g_timestamp = std::chrono::duration_cast<std::chrono::seconds>(current - start).count();
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
    std::cout << "Number of rules for incubator: " << subs.size() << "\n";
    if (subs.size() > 0)
    {
        std::cout << "\n";
        std::printf(subscription_format, "line", "ruleset", "rule", "type", "event", "field");
        std::cout << "--------------------------------------------------------------\n";
    }
    std::map<event_type_t, const char*> event_names;
    event_names[event_type_t::row_update] = "row update";
    event_names[event_type_t::row_insert] = "row insert";
    for (auto& s : subs)
    {
        std::printf(
            subscription_format,
            std::to_string(s->line_number).c_str(),
            s->ruleset_name,
            s->rule_name,
            std::to_string(s->gaia_type).c_str(),
            event_names[s->event_type],
            std::to_string(s->field).c_str());
    }
    std::cout << std::endl;
}

void add_fan_control_rule()
{
    try
    {
        subscribe_ruleset("incubator_ruleset");
    }
    catch (const duplicate_rule&)
    {
        std::cout << "The ruleset has already been added." << std::endl;
    }
}

void usage(const char* command)
{
    std::cout << "Usage: " << command << " [sim|show|help]\n";
    std::cout << " sim: run the incubator simulation.\n";
    std::cout << " show: dump the tables in storage.\n";
    std::cout << " help: print this message.\n";
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
            m_simulation_thread->join();
            std::cout << "Simulation stopped...\n";
        }
    }

    bool read_input()
    {
        std::getline(std::cin, m_input);
        return !std::cin.eof();
    }

    bool handle_main()
    {
        std::cout << "\n";
        std::cout << "(" << c_cmd_begin_sim << ") | begin simulation\n";
        std::cout << "(" << c_cmd_end_sim << ") | end simulation \n";
        std::cout << "(" << c_cmd_list_rules << ") | list rules\n";
        std::cout << "(" << c_cmd_disable_rules << ") | disable rules\n";
        std::cout << "(" << c_cmd_reenable_rules << ") | re-enable rules\n";
        std::cout << "(" << c_cmd_print_state << ") | print current state\n";
        std::cout << "(" << c_cmd_manage_incubators << ") | manage incubators\n";
        std::cout << "(" << c_cmd_quit << ") | quit\n\n";
        std::cout << "main> ";

        if (!read_input())
        {
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
                    m_simulation_thread = std::make_unique<std::thread>(simulation);
                    std::cout << "Simulation started...\n";
                }
                else
                {
                    std::cout << "Simulation is already running.\n";
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
                    std::cout << "Stopping simulation...\n";
                    g_in_simulation = false;
                    m_simulation_thread->join();
                    std::cout << "Simulation stopped...\n";
                }
                std::cout << "Exiting..." << std::endl;
                return false;
                break;
            default:
                wrong_input();
                break;
            }
        }
        else
        {
            wrong_input();
        }
        return true;
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
        std::cout << "\n";
        std::cout << "(" << c_cmd_choose_chickens << ") | select chicken incubator\n";
        std::cout << "(" << c_cmd_choose_puppies << ") | select puppy incubator\n";
        std::cout << "(" << c_cmd_back << ") | go back\n"
                  << std::endl;
        std::cout << "manage incubators> ";

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
                wrong_input();
                break;
            }
        }
        else
        {
            wrong_input();
        }

        return true;
    }

    bool handle_incubator_settings()
    {
        std::cout << "\n";
        std::cout << "(" << c_cmd_on << ")    | turn power on\n";
        std::cout << "(" << c_cmd_off << ")   | turn power off\n";
        std::cout << "(" << c_cmd_min << " #) | set minimum\n";
        std::cout << "(" << c_cmd_max << " #) | set maximum\n";
        std::cout << "(" << c_cmd_back << ")     | go back\n";
        std::cout << "(" << c_cmd_main << ")     | main menu\n"
                  << std::endl;
        std::cout << m_current_incubator_name << "> ";

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
                wrong_input();
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
                wrong_input();
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
                wrong_input();
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
            wrong_input();
            return true;
        }

        float set_point = 0;
        try
        {
            set_point = stof(m_input.substr(3, m_input.size() - 1));
        }
        catch (std::invalid_argument& ex)
        {
            std::cout << "Invalid temperature setting." << std::endl;
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
            auto w = m_current_incubator.writer();
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
                std::cout << "Max temp must be greater than min temp.\n";
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
    std::string m_input;
    incubator_t m_current_incubator;
    const char* m_current_incubator_name;
    std::unique_ptr<std::thread> m_simulation_thread;
    menu_t m_current_menu = menu_t::main;

    void wrong_input()
    {
        std::cerr << c_wrong_input << std::endl;
    };
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
            std::cerr << "Wrong arguments." << std::endl;
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

    std::cout << "-----------------------------------------\n";
    std::cout << "Gaia Incubator\n\n";
    std::cout << "No chickens or puppies were harmed in the\n";
    std::cout << "development or presentation of this demo.\n";
    std::cout << "-----------------------------------------\n";

    init_storage();
    sim.run();
    gaia::system::shutdown();
}
