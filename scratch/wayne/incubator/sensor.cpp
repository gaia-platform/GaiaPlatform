/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <unistd.h>

#include <cstring>
#include <ctime>

#include <algorithm>
#include <iostream>
#include <string>
#include <signal.h>

#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"
#include "gaia_incubator.h"
#include "incubator.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::direct_access;
using namespace gaia::incubator;
using namespace gaia::rules;

static volatile bool keep_running = true;

void usage(const char* command)
{
    printf("Usage: %s incubator-name sensor-name\n", command);
    printf("       where incubator-name is \"chicken\" or \"puppy\"\n");
    printf("       and sensor-name is unique\n");
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

void simulation(incubator_t& incubator, sensor_t& sensor)
{
    auto_transaction_t tx;

    while (keep_running)
    {
        sleep(1);

        float new_temp = 0.0;
        int count = 0;
        // To prevent divergence of temperatures of multiple sensors, start with the average of all sensor values.
        for (const auto& s : incubator.sensor_list())
        {
            new_temp += s.value();
            count++;
        }
        new_temp /= count;

        sensor_writer sw = sensor.writer();
        for (const auto& a : incubator.actuator_list())
        {
            float fan = a.value();
            new_temp = calc_new_temp(new_temp, fan);
            sw.value = new_temp;
            sw.timestamp = time(nullptr) - incubator.start_time();
        }
        sw.update_row();

        tx.commit();
    }

}

bool init_storage(const char* incubator_name, const char* sensor_name, incubator_t& incubator, sensor_t& sensor)
{
    begin_transaction();
    for (auto i : incubator_t::list())
    {
        if (strcmp(i.name(), incubator_name) == 0)
        {
            sensor = sensor_t::get(sensor_t::insert_row(sensor_name, 0, i.min_temp()));
            i.sensor_list().insert(sensor);
            printf("added '%s' to '%s'\n", sensor_name, incubator_name);
            commit_transaction();
            incubator = i;
            return true;
        }
    }
    rollback_transaction();
    return false;
}

void remove_sensor(incubator_t& incubator, sensor_t& sensor)
{
    begin_transaction();
    printf("removed '%s' from '%s'\n", sensor.name(), incubator.name());
    incubator.sensor_list().erase(sensor);
    sensor.delete_row();
    commit_transaction();
    return;
}

void intHandler(int) {
    keep_running = 0;
    printf("Terminating sensor\n");
}

int main(int argc, const char** argv)
{
    const char* c_arg_help = "help";

    if (argc == 2 && strncmp(argv[1], c_arg_help, strlen(c_arg_help)) == 0)
    {
        usage(argv[0]);
        return EXIT_SUCCESS;
    }
    else if (argc != 3)
    {
        cout << "Wrong arguments." << endl;
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    gaia::system::initialize("gaia.conf", "gaia_log.conf");

    printf("-----------------------------------------\n");
    printf("Gaia Incubator\n\n");
    printf("No chickens or puppies were harmed in the\n");
    printf("development or presentation of this demo.\n");
    printf("-----------------------------------------\n");

    signal(SIGINT, intHandler);

    incubator_t incubator;
    sensor_t sensor;
    if (init_storage(argv[1], argv[2], incubator, sensor))
    {
        simulation(incubator, sensor);
        remove_sensor(incubator, sensor);
    }

    gaia::system::shutdown();
}
