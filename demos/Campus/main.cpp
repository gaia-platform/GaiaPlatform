/*#include "barn_storage_gaia_generated.h"
#include "events.hpp"
#include "gaia_system.hpp"
#include "rules.hpp"
#include <algorithm>
#include <atomic>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>

using namespace std;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::rules;
using namespace BarnStorage;

const double FAN_SPEED_LIMIT = 3500.0;
const char SENSOR_A_NAME[] = "Temp A";
const char SENSOR_B_NAME[] = "Temp B";
const char SENSOR_C_NAME[] = "Temp C";
const char ACTUATOR_A_NAME[] = "Fan A";
const char ACTUATOR_B_NAME[] = "Fan B";
const char ACTUATOR_C_NAME[] = "Fan C";
atomic<bool> IN_SIMULATION{false};
atomic<int> TIMESTAMP{0};

void add_fan_control_rule();

void init_storage() {
    begin_transaction();

    ulong gaia_id;

    gaia_id = Incubator::insert_row("Chicken", 99.0, 102.0);
    Sensor::insert_row(gaia_id, SENSOR_A_NAME, 0, 99.0);
    Sensor::insert_row(gaia_id, SENSOR_C_NAME, 0, 99.0);
    Actuator::insert_row(gaia_id, ACTUATOR_A_NAME, 0, 0.0);

    gaia_id = Incubator::insert_row("Puppy", 85.0, 90.0);
    Sensor::insert_row(gaia_id, SENSOR_B_NAME, 0, 85.0);
    Actuator::insert_row(gaia_id, ACTUATOR_B_NAME, 0, 0.0);
    Actuator::insert_row(gaia_id, ACTUATOR_C_NAME, 0, 0.0);

    commit_transaction();
}

void dump_db() {
    begin_transaction();
    printf("\n");

    printf("[Incubators]\n");
    for (auto i = Incubator::get_first(); i;
         i = i.get_next()) {
        printf("  name  | min_temp | max_temp \n");
        printf("------------------------------\n");
        printf("%-8s|%10.1lf|%10.1lf\n", i.name(), i.min_temp(),
               i.max_temp());
        printf("\n");
        printf("  [Sensors]\n");
        printf("   name | time | value \n");
        printf("  ---------------------\n");
        for (Sensor s = Sensor::get_first(); s;
             s = s.get_next()){
            if (s.incubator_id() == i.gaia_id()) {
                printf("  %-6s|%6ld|%7.1lf\n", s.name(), s.timestamp(),
                       s.value());
            }
        }
        printf("\n");
        printf("  [Actuators]\n");
        printf("   name | time | value \n");
        printf("  ---------------------\n");
        for (Actuator a = Actuator::get_first(); a;
             a = a.get_next()) {
            if (a.incubator_id() == i.gaia_id()) {
                printf("  %-6s|%6ld|%7.1lf\n", a.name(), a.timestamp(),
                       a.value());
            }
        }
        printf("\n");
        printf("\n");
    }
    commit_transaction();
}

double calc_new_temp(float curr_temp, float fan_speed) {
    if (fan_speed < 1000) {
        return curr_temp + 0.1;
    } else if (fan_speed < 3000) {
        return curr_temp - 0.1;
    }
    return curr_temp - 0.2;
}

void simulation() {
    time_t start, cur;
    time(&start);
    while (IN_SIMULATION) {
        sleep(1);
        begin_transaction();
        double new_temp, fa_v, fb_v, fc_v;
        for (Actuator a = Actuator::get_first(); 
            a;
            a = a.get_next()) {
            if (strcmp(a.name(), ACTUATOR_A_NAME) == 0) {
                fa_v = a.value();
            } else if (strcmp(a.name(), ACTUATOR_B_NAME) == 0) {
                fb_v = a.value();
            } else if (strcmp(a.name(), ACTUATOR_C_NAME) == 0) {
                fc_v = a.value();
            }
        }
        time(&cur);
        TIMESTAMP = difftime(cur, start);
        for (Sensor s = Sensor::get_first(); 
            s;
            s = s.get_next()) {
            Sensor_writer w = s.writer();
            if (strcmp(s.name(), SENSOR_A_NAME) == 0) {
                new_temp = calc_new_temp(s.value(), fa_v);
                w.value = new_temp;
                w.timestamp = TIMESTAMP;
                w.update_row();
            } else if (strcmp(s.name(), SENSOR_B_NAME) == 0) {
                new_temp = calc_new_temp(s.value(), fb_v);
                new_temp = calc_new_temp(new_temp, fc_v);
                w.value = new_temp;
                w.timestamp = TIMESTAMP;
                w.update_row();
            } else if (strcmp(s.name(), SENSOR_C_NAME) == 0) {
                new_temp = calc_new_temp(s.value(), fa_v);
                w.value = new_temp;
                w.timestamp = TIMESTAMP;
                w.update_row();
            }
        }
        commit_transaction();
    }
}

void decrease_fans(Incubator& incubator, FILE *log) {
    fprintf(log, "%s called for %s incubator.\n", __func__, incubator.name());
    for (Actuator a = Actuator::get_first(); 
        a;
        a = a.get_next()) {
        if (a.incubator_id() == incubator.gaia_id()) {
            Actuator_writer w = a.writer();
            w.value = max(0.0, w.value - 500.0);
            w.timestamp = TIMESTAMP;
            w.update_row();
        }
    }
}

void increase_fans(Incubator& incubator, FILE *log) {
    fprintf(log, "%s called for %s incubator.\n", __func__, incubator.name());
    for (Actuator a = Actuator::get_first(); 
         a;
         a = a.get_next()) {
        if (a.incubator_id() == incubator.gaia_id()) {
            Actuator_writer w = a.writer();
            w.value = min(FAN_SPEED_LIMIT, w.value + 500.0);
            w.timestamp = TIMESTAMP;
            w.update_row();
        }
    }
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
    map<event_type_t, const char *> event_names;
    event_names[event_type_t::row_update] = "Row update";
    event_names[event_type_t::row_insert] = "Row insert";
    for (auto &s : subs) {
        printf("%-10s|%-11s|%-7s\n", s->ruleset_name, s->rule_name,
               event_names[s->type]);
    }
    printf("\n");
}

void usage(const char *command) {
    printf("Usage: %s [sim|show|help]\n", command);
    printf(" sim: run the incubator simulation.\n");
    printf(" show: dump the tables in storage.\n");
    printf(" help: print this message.\n");
}

int main(int argc, const char **argv) {
    bool is_sim = false;

    if (argc == 2 && strncmp(argv[1], "sim", 3) == 0) {
        is_sim = true;
    } else if (argc == 2 && strncmp(argv[1], "show", 3) == 0) {
        is_sim = false;
    } else if (argc == 2 && strncmp(argv[1], "help", 3) == 0) {
        usage(argv[0]);
        return EXIT_SUCCESS;
    } else {
        cout << "Wrong arguments." << endl;
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (!is_sim) {
        gaia_mem_base::init(false);
        dump_db();
        return EXIT_SUCCESS;
    }

    printf("Incubator simulation...\n");
    gaia::system::initialize(true);
    init_storage();
    dump_db();
    string input;
    thread simulation_thread[1];
    while (true) {
        printf("(s)tart / s(t)op / (l)ist rules / (a)dd fan control rule / "
               "(c)lear rules / (e)xit: ");
        getline(cin, input);
        if (input.size() == 1) {
            switch (input[0]) {
            case 's':
                if (!IN_SIMULATION) {
                    IN_SIMULATION = true;
                    simulation_thread[0] = thread(simulation);
                    printf("Simulation started...\n");
                } else {
                    printf("Simulation is already running.\n");
                }
                break;
            case 't':
                if (IN_SIMULATION) {
                    IN_SIMULATION = false;
                    simulation_thread[0].join();
                    printf("Simulation stopped...\n");
                } else {
                    printf("Simulation is not started.\n");
                }
                break;
            case 'l':
                list_rules();
                break;
            case 'a':
                add_fan_control_rule();
                break;
            case 'c':
                unsubscribe_rules();
                break;
            case 'e':
                if (IN_SIMULATION) {
                    printf("Stopping simulation...\n");
                    IN_SIMULATION = false;
                    simulation_thread[0].join();
                    printf("Simulation stopped...\n");
                }
                printf("Exiting...");
                return EXIT_SUCCESS;
                break;
            default:
                printf("Wrong input.");
                break;
            }
        }
    }

    return EXIT_SUCCESS;
}*/

int main(int argc, const char **argv) {
    bool is_sim = false;

    return 0;
}

