#include "gaia_incubator.h"
#include "gaia/system.hpp"
#include "gaia/rules/rules.hpp"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

using namespace std;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::rules;
using namespace gaia::incubator;

const char* c_sensor_a = "Temp A";
const char* c_sensor_b = "Temp B";
const char* c_sensor_c = "Temp C";
const char* c_actuator_a = "Fan A";
const char* c_actuator_b = "Fan B";
const char* c_actuator_c = "Fan C";

const char* c_chicken = "chicken";
const float c_chicken_min = 99.0;
const float c_chicken_max = 102.0;

const char* c_puppy = "puppy";
const float c_puppy_min = 85.0;
const float c_puppy_max = 100.0;

atomic<bool> IN_SIMULATION{false};
atomic<int> TIMESTAMP{0};

void add_fan_control_rule();

gaia_id_t insert_incubator(const char* name, float min_temp, float max_temp) {
    incubator_writer w;
    w.name = name;
    w.is_on = false;
    w.min_temp = min_temp;
    w.max_temp = max_temp;
    return w.insert_row();
}

void restore_sensor(sensor_t& sensor, float min_temp) {
    sensor_writer w = sensor.writer();
    w.timestamp = 0;
    w.value = min_temp;
    w.update_row();
}

void restore_actuator(actuator_t& actuator) {
    actuator_writer w = actuator.writer();
    w.timestamp = 0;
    w.value = 0.0;
    w.update_row();
}

void restore_incubator(incubator_t& incubator, float min_temp, float max_temp) {
    incubator_writer w = incubator.writer();
    w.is_on = false;
    w.min_temp = min_temp;
    w.max_temp = max_temp;
    w.update_row();

    for (auto& sensor : incubator.sensor_list()) {
        restore_sensor(sensor, min_temp);
    }

    for (auto& actuator : incubator.actuator_list()) {
        restore_actuator(actuator);
    }
}

void restore_default_values() {
    for (auto& incubator : incubator_t::list()) {
        float min_temp, max_temp;
        if (strcmp(incubator.name(), c_chicken) == 0) {
            min_temp = c_chicken_min;
            max_temp = c_chicken_max;
        }
        else {
            min_temp = c_puppy_min;
            max_temp = c_puppy_max;
        }
        restore_incubator(incubator, min_temp, max_temp);
    }
}

void init_storage() {
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);

    // If we already have inserted an incubator then our storage has already been
    // initialized.  Re-initialize the database to default values.
    if (incubator_t::get_first()) {
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
    incubator.sensor_list().insert(sensor_t::insert_row(c_sensor_b, 0, 85.0));
    incubator.actuator_list().insert(actuator_t::insert_row(c_actuator_b, 0, 0.0));
    incubator.actuator_list().insert(actuator_t::insert_row(c_actuator_c, 0, 0.0));

    tx.commit();
}

void dump_db() {
    begin_transaction();
    printf("\n");

    for (auto i : incubator_t::list()) {
        printf("-----------------------------------------\n");
        printf("%-8s|power: %-3s|min: %5.1lf|max: %5.1lf\n", 
            i.name(), i.is_on() ? "ON" : "OFF", i.min_temp(), i.max_temp());
        printf("-----------------------------------------\n");
        for (auto s : i.sensor_list()) {
            printf("\t|%-10s|%10ld|%10.2lf\n", s.name(), s.timestamp(), s.value());
        }
        printf("\t---------------------------------\n");
        for (auto a : i.actuator_list()) {
            printf("\t|%-10s|%10ld|%10.1lf\n", a.name(), a.timestamp(), a.value());
        }
        printf("\n");
        printf("\n");
    }
    commit_transaction();
}

double calc_new_temp(float curr_temp, float fan_speed) {

    if (fan_speed == 0) {
        return curr_temp + 0.1;
    }
    else if (fan_speed < 1000) {
        return curr_temp - 0.025;
    } 
    else if (fan_speed < 1500) {
        return curr_temp - 0.05;
    } 
    else if (fan_speed < 2000) {
        return curr_temp - 0.075;
    } 
    else if (fan_speed < 2500) {
        return curr_temp - 0.1;
    }        
    else if (fan_speed < 3000) {
        return curr_temp - 0.125;
    }
    else if (fan_speed < 3500) {
        return curr_temp - 0.150;
    }
    else if (fan_speed < 4000) {
        return curr_temp - 0.175;
    }
    return curr_temp - 0.2;
}

void set_power(bool is_on)
{
    begin_transaction();
    for (auto i : incubator_t::list())
    {
        auto w = i.writer();
        w.is_on = is_on;
        w.update_row();
    }
    commit_transaction();
}

void simulation() {
    time_t start, cur;
    time(&start);
    begin_session();
    set_power(true);
    while (IN_SIMULATION) {
        sleep(1);
        begin_transaction();
        double new_temp, fa_v, fb_v, fc_v;
        for (auto a : actuator_t::list()) {
            if (strcmp(a.name(), c_actuator_a) == 0) {
                fa_v = a.value();
            } else if (strcmp(a.name(), c_actuator_b) == 0) {
                fb_v = a.value();
            } else if (strcmp(a.name(), c_actuator_c) == 0) {
                fc_v = a.value();
            }
        }
        time(&cur);
        TIMESTAMP = difftime(cur, start);
        for (auto s : sensor_t::list()) {
            sensor_writer w = s.writer();
            if (strcmp(s.name(), c_sensor_a) == 0) {
                new_temp = calc_new_temp(s.value(), fa_v);
                w.value = new_temp;
                w.timestamp = TIMESTAMP;
                w.update_row();
            } else if (strcmp(s.name(), c_sensor_b) == 0) {
                new_temp = calc_new_temp(s.value(), fb_v);
                new_temp = calc_new_temp(new_temp, fc_v);
                w.value = new_temp;
                w.timestamp = TIMESTAMP;
                w.update_row();
            } else if (strcmp(s.name(), c_sensor_c) == 0) {
                new_temp = calc_new_temp(s.value(), fa_v);
                w.value = new_temp;
                w.timestamp = TIMESTAMP;
                w.update_row();
            }
        }
        commit_transaction();
    }
    set_power(false);
    end_session();
}

void list_rules() {
    subscription_list_t subs;
    list_subscribed_rules(nullptr, nullptr, nullptr, nullptr, subs);
    printf("Number of rules for incubator: %ld\n", subs.size());
    if (subs.size() > 0) {
        printf("\n");
        printf(" rule set | rule name | event \n");
        printf("------------------------------\n");
    }
    map<event_type_t, const char*> event_names;
    event_names[event_type_t::row_update] = "Row update";
    event_names[event_type_t::row_insert] = "Row insert";
    for (auto &s : subs) {
        printf("%-10s|%-11s|%-7s\n", s->ruleset_name, s->rule_name,
               event_names[s->event_type]);
    }
    printf("\n");
}

void add_fan_control_rule() {
    try {
        subscribe_ruleset("incubator_ruleset");
    } catch (duplicate_rule) {
        printf("The ruleset has already been added.\n");
    }
}


void usage(const char*command) {
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


    bool handle_main() {
        printf("\n");
        printf("(%c) | start simulation\n", c_cmd_begin_sim);
        printf("(%c) | end simulation \n", c_cmd_end_sim);
        printf("(%c) | list rules\n", c_cmd_list_rules);
        printf("(%c) | disable rules\n", c_cmd_disable_rules);
        printf("(%c) | re-enable rules\n", c_cmd_reenable_rules);
        printf("(%c) | print current state\n", c_cmd_print_state);
        printf("(%c) | manage incubators\n", c_cmd_manage_incubators);
        printf("(%c) | quit\n\n", c_cmd_quit);
        printf("main> ");

        getline(cin, m_input);
        if (m_input.size() == 1) {
            switch (m_input[0]) {
            case c_cmd_begin_sim:
                if (!IN_SIMULATION) {
                    IN_SIMULATION = true;
                    m_simulation_thread[0] = thread(simulation);
                    printf("Simulation started...\n");
                } else {
                    printf("Simulation is already running.\n");
                }
                break;
            case c_cmd_end_sim:
                if (IN_SIMULATION) {
                    IN_SIMULATION = false;
                    m_simulation_thread[0].join();
                    printf("Simulation stopped...\n");
                } else {
                    printf("Simulation is not started.\n");
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
                m_current_menu = 1;
                break;
            case c_cmd_print_state:
                dump_db();
                break;
            case c_cmd_quit:
                if (IN_SIMULATION) {
                    printf("Stopping simulation...\n");
                    IN_SIMULATION = false;
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

    void get_incubator(const char* name)
    {
        begin_transaction();
        for (auto inc : incubator_t::list())
        {
            if (strcmp(inc.name(), name) == 0) {
                m_current_incubator = inc;
                break;
            }
        }
        commit_transaction();
        m_current_incubator_name = name;
    }

    void handle_incubators() {
        printf("\n");
        printf("(%c) | select chicken incubator\n", c_cmd_choose_chickens);
        printf("(%c) | select puppy incubator\n", c_cmd_choose_puppies);
        printf("(%c) | go back\n\n", c_cmd_back);
        printf("manage incubators> ");
        getline(cin, m_input);
        if (m_input.size() == 1) {
            switch (m_input[0]) {
            case c_cmd_choose_chickens:
                get_incubator(c_chicken);
                m_current_menu = 2;
                break;
            case c_cmd_choose_puppies:
                get_incubator(c_puppy);
                m_current_menu = 2;
                break;
            case c_cmd_back:
                m_current_menu = 0;
                break;
            default:
                printf("%s\n", c_wrong_input);
                break;
            }
        }
    }

    void handle_incubator_settings() {
        printf("\n");
        printf("(%s)    | turn power on\n", c_cmd_on);
        printf("(%s)   | turn power off\n", c_cmd_off);
        printf("(%s #) | set minimum\n", c_cmd_min);
        printf("(%s #) | set maximum\n", c_cmd_max);
        printf("(%c)     | go back\n", c_cmd_back);
        printf("(%c)     | main menu\n\n", c_cmd_main);
        printf("%s> ", m_current_incubator_name);
        getline(cin, m_input);

        if (m_input.size() == 1) {
            switch (m_input[0]) {
            case c_cmd_back:
                m_current_menu = 1;
                break;
            case c_cmd_main:
                m_current_menu = 0;
            default:
                printf("%s\n", c_wrong_input);
                break;
            }
            return;
        }

        if (m_input.size() == strlen(c_cmd_on)) {
            if (0 == m_input.compare(c_cmd_on)) {
                adjust_power(true);
                dump_db();
            }
            else {
                printf("%s\n", c_wrong_input);
            }
            return;
        }

        if (m_input.size() == strlen(c_cmd_off)) {
            if (0 == m_input.compare(c_cmd_off)) {
                adjust_power(false);
                dump_db();
            }
            else {
                printf("%s\n", c_wrong_input);
            }
            return;
        }

        bool change_min = false;
        if (0 == m_input.compare(0, 3, c_cmd_min)) {
            change_min = true;
        }
        else if (0 == m_input.compare(0,3, c_cmd_max)) {
            change_min = false;
        }
        else {
            printf("%s\n", c_wrong_input);
            return;
        }

        float set_point = 0;
        try {
            set_point = stof(m_input.substr(3, m_input.size() -1));
        }
        catch(invalid_argument& ex) {
            printf("Invalid temperature setting.\n");
            return;
        }

        if (adjust_range(change_min, set_point)) {
            dump_db();
        }
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
            if (change_min) {
                w.min_temp = set_point;
            }
            else {
                w.max_temp = set_point;
            }
            if (w.min_temp >= w.max_temp) {
                printf("Max temp must be greater than min temp.\n");
            }
            else {
                w.update_row();
                changed = true;
            }
        }
        commit_transaction();
        return changed;
    }

    int run() {
        while (true) {
            switch(m_current_menu) {
                case 0:
                    if (!handle_main()) {
                        return EXIT_SUCCESS;
                    }
                    break;
                case 1:
                    handle_incubators();
                    break;
                case 2:
                    handle_incubator_settings();
                    break;
                default:
                    // do nothing
                    break;
            }
        }
    }

private:
    string m_input;
    incubator_t m_current_incubator;
    const char* m_current_incubator_name;
    thread m_simulation_thread[1];
    int m_current_menu = 0;
};

int main(int argc, const char**argv) {
    bool is_sim = false;
    std::string server;
    const char * c_arg_sim = "sim";
    const char * c_arg_show = "show";
    const char * c_arg_help = "help";

    if (argc == 2 && strncmp(argv[1], c_arg_sim, strlen(c_arg_sim)) == 0) {
        is_sim = true;
    } else if (argc == 2 && strncmp(argv[1], c_arg_show, strlen(c_arg_show)) == 0) {
        is_sim = false;
    } else if (argc == 2 && strncmp(argv[1], c_arg_help, strlen(c_arg_help)) == 0) {
        usage(argv[0]);
        return EXIT_SUCCESS;
    } else {
        cout << "Wrong arguments." << endl;
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (!is_sim) {
        gaia::system::initialize();
        gaia::rules::unsubscribe_rules();
        dump_db();
        return EXIT_SUCCESS;
    }

    simulation_t sim;
    gaia::system::initialize("./gaia.conf");

    printf("-----------------------------------------\n");
    printf("Gaia Incubator\n\n");
    printf("No chickens or puppies were harmed in the\n");
    printf("development or presentation of this demo.\n");
    printf("-----------------------------------------\n");

    init_storage();
    sim.run();
}
