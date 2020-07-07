#include <memory>
#include <chrono>
#include <functional>

#include "gaia_incubator/incubator_manager.hpp"

int main(int argc, char* argv[])
{
    cout << "Starting incubator_manager." << endl;
    init(argc, argv);
    spin(make_shared<incubator_manager>());

    shutdown();
    cout << "Shut down incubator_manager." << endl;
    return 0;
}

incubator_manager::incubator_manager(): Node("incubator_manager")
{
    using namespace chrono_literals;
    using std::placeholders::_1;

    m_pub_temp =
        this->create_publisher<msg::Temp>("temp", SensorDataQoS());

    m_sub_fan_state =
        this->create_subscription<msg::FanState>(
            "fan_state", SystemDefaultsQoS(), std::bind(
                &incubator_manager::set_fan_state, this, _1));

    m_sub_add_incubator =
        this->create_subscription<msg::AddIncubator>(
            "add_incubator", SystemDefaultsQoS(), std::bind(
                &incubator_manager::add_incubator, this, _1));

    m_sub_add_sensor =
        this->create_subscription<msg::AddSensor>(
            "add_sensor", SystemDefaultsQoS(), std::bind(
                &incubator_manager::add_sensor, this, _1));

    m_sub_add_fan =
        this->create_subscription<msg::AddFan>(
            "add_fan", SystemDefaultsQoS(), std::bind(
                &incubator_manager::add_fan, this, _1));

    m_sensor_reading_timer = this->create_wall_timer(
        1s, bind(&incubator_manager::publish_temp, this));

    m_update_state_timer = this->create_wall_timer(
        100ms, bind(&incubator_manager::update_state, this));
} // incubator_manager()

void incubator_manager::update_state() {
    lock_guard<mutex> incubators_lock(m_incubators_mutex);

    for(auto& incubator_pair : m_incubators)
    {
        incubator& current_incubator = incubator_pair.second;

        float temp_change = 0.01;

        for(auto& fan_pair : current_incubator.fans)
        {
            fan& current_fan = fan_pair.second;

            // Calculate the new fan speed.
            if(current_fan.is_on)
            {
                current_fan.speed = min(3500.0, current_fan.speed + 50.0);
            }
            else
            {
                current_fan.speed = max(0.0, current_fan.speed - 50.0);
            }

            // Calculate the temperature change for the current fan.
            if(current_fan.speed > 3000.0)
                temp_change -= 0.03;
            else if(current_fan.speed > 1000.0)
                temp_change -= 0.02;
        } // for loop, fans

        current_incubator.temperature += temp_change;
    } // for loop, incubators
} // update_state()

void incubator_manager::publish_temp()
{
    msg::Temp temp_msg;
    lock_guard<mutex> incubators_lock(m_incubators_mutex);

    for(const auto& incubator_pair : m_incubators)
    {
        temp_msg.incubator_id = incubator_pair.first;

        const incubator& current_incubator = incubator_pair.second;

        for(const string& sensor_name : current_incubator.sensors)
        {
            temp_msg.sensor_name = sensor_name;
            // Every sensor in an incubator reports the same temperature.
            temp_msg.value = current_incubator.temperature;
            m_pub_temp->publish(temp_msg);
        }
    }
}

void incubator_manager::set_fan_state(const msg::FanState::SharedPtr msg)
{
    lock_guard<mutex> incubators_lock(m_incubators_mutex);
    const auto incubator_iter = m_incubators.find(msg->incubator_id);

    if(incubator_iter == m_incubators.end())
    {
        RCLCPP_INFO(get_logger(),
            "Failed to set a fan's state: cannot find an incubator with ID %u.",
            msg->incubator_id);
    }
    else
    {
        const auto fan_iter = incubator_iter->second.fans.find(msg->fan_name);

        if(fan_iter == incubator_iter->second.fans.end())
        {
            RCLCPP_INFO(get_logger(),
                "Failed to set a fan's state: cannot find a fan named %s in the incubator with ID %u.",
                msg->fan_name.c_str(),
                msg->incubator_id);
        }
        else
        {
            fan_iter->second.is_on = msg->is_on;
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
        m_incubators.at(msg->incubator_id).sensors.insert(
            msg->sensor_name);
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
