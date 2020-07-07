#pragma once

#include <iostream>
#include <set>
#include <map>
#include <mutex>

#include "rclcpp/rclcpp.hpp"

#include "gaia_incubator/msg/temp.hpp"
#include "gaia_incubator/msg/fan_state.hpp"
#include "gaia_incubator/msg/add_incubator.hpp"
#include "gaia_incubator/msg/add_sensor.hpp"
#include "gaia_incubator/msg/add_fan.hpp"

using namespace std;
using namespace rclcpp;
using namespace gaia_incubator;

class incubator_manager : public Node
{
public:
    incubator_manager();

private:
    void update_state();

    void publish_temp();

    void set_fan_state(const msg::FanState::SharedPtr msg);

    void add_incubator(const msg::AddIncubator::SharedPtr msg);
    void add_sensor(const msg::AddSensor::SharedPtr msg);
    void add_fan(const msg::AddFan::SharedPtr msg);

    Publisher<msg::Temp>::SharedPtr m_pub_temp;

    Subscription<msg::FanState>::SharedPtr m_sub_fan_state;
    Subscription<msg::AddIncubator>::SharedPtr m_sub_add_incubator;
    Subscription<msg::AddSensor>::SharedPtr m_sub_add_sensor;
    Subscription<msg::AddFan>::SharedPtr m_sub_add_fan;

    TimerBase::SharedPtr m_sensor_reading_timer;
    TimerBase::SharedPtr m_update_state_timer;

    struct fan
    {
        float speed = 0.0;
        bool is_on = false;
    };

    struct incubator
    {
        string name;
        float temperature;

        // Sensors are only stored as their names. They do not keep a state.
        set<string> sensors;
        // Fans are identified by their names.
        map<string, fan> fans;
    };

    // Incubators are identified by their gaia_id, which is the uint32_t.
    map<uint32_t, incubator> m_incubators;
    mutex m_incubators_mutex;
};
