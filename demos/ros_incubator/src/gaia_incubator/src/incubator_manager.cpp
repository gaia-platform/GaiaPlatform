/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <memory>
#include <chrono>
#include <functional>

#include "gaia_incubator/incubator_manager.hpp"

incubator_manager::incubator_manager(const NodeOptions& options)
: Node("incubator_manager", options)
{
    cout << "Starting incubator_manager." << endl;
    on_shutdown([&]
    {
        incubator_manager::shutdown_callback();
    });

    using namespace chrono_literals;
    using std::placeholders::_1;

    m_pub_temp = this->create_publisher<msg::Temp>("temp", SensorDataQoS());

    m_sub_fan_state = this->create_subscription<msg::FanState>(
        "fan_state", SystemDefaultsQoS(), std::bind(
            &incubator_manager::set_fan_state, this, _1));

    m_sub_add_incubator = this->create_subscription<msg::AddIncubator>(
        "add_incubator", SystemDefaultsQoS(), std::bind(
            &incubator_manager::add_incubator, this, _1));

    m_sub_add_sensor = this->create_subscription<msg::AddSensor>(
        "add_sensor", SystemDefaultsQoS(), std::bind(
            &incubator_manager::add_sensor, this, _1));

    m_sub_add_fan = this->create_subscription<msg::AddFan>(
        "add_fan", SystemDefaultsQoS(), std::bind(
            &incubator_manager::add_fan, this, _1));

    m_sensor_reading_timer = this->create_wall_timer(
        (1s * c_publish_temp_rate),
        bind(&incubator_manager::publish_temp, this));

    m_update_state_timer = this->create_wall_timer(
        (1s * c_update_state_rate),
        bind(&incubator_manager::update_state, this));
} // incubator_manager()

void incubator_manager::update_state() {
    lock_guard<mutex> incubators_lock(m_incubators_mutex);

    for (auto& incubator_pair : m_incubators)
    {
        incubator& current_incubator = incubator_pair.second;

        double temp_change = c_temp_change_initial;

        for (auto& fan_pair : current_incubator.fans)
        {
            fan& current_fan = fan_pair.second;

            // Calculate the new fan speed.
            if (current_fan.is_on)
            {
                current_fan.speed = min(c_fan_max_speed,
                    c_fan_acceleration * c_update_state_rate
                    + current_fan.speed);
            }
            else
            {
                current_fan.speed = max(0.0,
                    c_fan_acceleration * c_update_state_rate
                    - current_fan.speed);
            }

            // Calculate the temperature change for the current fan.
            if (current_fan.speed > c_fan_high_speed)
            {
                temp_change -= c_temp_change_high_fan_speed;
            }
            else if (current_fan.speed > c_fan_low_speed)
            {
                temp_change -= c_temp_change_low_fan_speed;
            }
        } // for : current_incubator.fans

        current_incubator.temperature += temp_change;
    } // for : m_incubators
} // update_state()

void incubator_manager::publish_temp()
{
    msg::Temp temp_msg;
    lock_guard<mutex> incubators_lock(m_incubators_mutex);

    for (const auto& incubator_pair : m_incubators)
    {
        temp_msg.incubator_id = incubator_pair.first;

        const incubator& current_incubator = incubator_pair.second;

        for (const string& sensor_name : current_incubator.sensors)
        {
            temp_msg.sensor_name = sensor_name;
            // Every sensor in an incubator reports the same temperature.
            temp_msg.value = current_incubator.temperature;
            temp_msg.stamp = now();

            m_pub_temp->publish(temp_msg);
        }
    }
}

void incubator_manager::set_fan_state(const msg::FanState::SharedPtr msg)
{
    lock_guard<mutex> incubators_lock(m_incubators_mutex);
    const auto incubator_iter = m_incubators.find(msg->incubator_id);

    if (incubator_iter == m_incubators.end())
    {
        RCLCPP_INFO(get_logger(),
            "Failed to set fan states: cannot find an incubator with ID %u.",
            msg->incubator_id);
    }
    else
    {
        for (auto& fan_pair : incubator_iter->second.fans)
        {
            fan& current_fan = fan_pair.second;
            current_fan.is_on = msg->fans_on;
        }
    }
}

void incubator_manager::add_incubator(const msg::AddIncubator::SharedPtr msg)
{
    incubator inc;
    inc.name = msg->name;
    inc.temperature = msg->temperature;

    lock_guard<mutex> incubators_lock(m_incubators_mutex);
    m_incubators[msg->incubator_id] = inc;
}

void incubator_manager::add_sensor(const msg::AddSensor::SharedPtr msg)
{
    try
    {
        lock_guard<mutex> incubators_lock(m_incubators_mutex);
        m_incubators.at(msg->incubator_id).sensors.insert(msg->sensor_name);
    }
    catch(const out_of_range)
    {
        RCLCPP_INFO(get_logger(),
            "Failed to add a sensor: cannot find an incubator with ID %u.",
            msg->incubator_id);
    }
}

void incubator_manager::add_fan(const msg::AddFan::SharedPtr msg)
{
    try
    {
        fan f;

        lock_guard<mutex> incubators_lock(m_incubators_mutex);
        m_incubators.at(msg->incubator_id).fans[msg->fan_name] = f;
    }
    catch(const out_of_range)
    {
        RCLCPP_INFO(get_logger(),
            "Failed to add a fan: cannot find an incubator with ID %u.",
            msg->incubator_id);
    }
}

void incubator_manager::shutdown_callback()
{
    cout << "Shut down incubator_manager." << endl;
}
