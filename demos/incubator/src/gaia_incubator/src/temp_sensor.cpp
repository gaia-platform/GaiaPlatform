#include <chrono>
#include <functional>
#include <memory>
#include <iostream>
#include <set>
#include <map>

#include "rclcpp/rclcpp.hpp"

// ROS messages
#include "gaia_incubator/msg/temp.hpp"
#include "gaia_incubator/msg/fan_state.hpp"
#include "gaia_incubator/msg/add_incubator.hpp"
#include "gaia_incubator/msg/add_sensor.hpp"
#include "gaia_incubator/msg/add_fan.hpp"

using namespace std;
using namespace rclcpp;

class TempSensor : public Node
{
public:
    TempSensor(): Node("temp_sensor")
    {
        using namespace chrono_literals;
        using std::placeholders::_1;

        pub_temp =
            this->create_publisher<gaia_incubator::msg::Temp>("temp", 10);

        sub_fan_state =
            this->create_subscription<gaia_incubator::msg::FanState>(
                "fan_state", 10, std::bind(
                    &TempSensor::set_fan_state, this, _1));

        sub_add_incubator =
            this->create_subscription<gaia_incubator::msg::AddIncubator>(
                "add_incubator", 10, std::bind(
                    &TempSensor::add_incubator, this, _1));

        sub_add_sensor =
            this->create_subscription<gaia_incubator::msg::AddSensor>(
                "add_sensor", 10, std::bind(
                    &TempSensor::add_sensor, this, _1));

        sub_add_fan =
            this->create_subscription<gaia_incubator::msg::AddFan>(
                "add_fan", 10, std::bind(
                    &TempSensor::add_fan, this, _1));

        sensor_reading_timer = this->create_wall_timer(
            1s, bind(&TempSensor::publish_temp, this));

        update_state_timer = this->create_wall_timer(
            100ms, bind(&TempSensor::update_state, this));
    }

private:
    void update_state() {
        for(auto& incubator_pair : incubators)
        {
            incubator& current_incubator = incubator_pair.second;
            float temp_change = 0.01;

            for(auto& fan_pair : current_incubator.fans)
            {
                fan& current_fan = fan_pair.second;

                // calculate new fan speed
                if(current_fan.on)
                    current_fan.speed = min(3500.0, current_fan.speed + 50.0);
                else
                    current_fan.speed = max(0.0, current_fan.speed - 50.0);

                // calculate temperature change per fan
                if(current_fan.speed > 3000.0)
                    temp_change -= 0.03;
                else if(current_fan.speed > 1000.0)
                    temp_change -= 0.02;
            } // for fans

            current_incubator.temperature += temp_change;
        } // for incubators
    } // update_state()

    void publish_temp()
    {
        auto temp_msg = gaia_incubator::msg::Temp();

        for(const auto& incubator_pair : incubators)
        {
            temp_msg.incubator_id = incubator_pair.first;
            const incubator& current_incubator = incubator_pair.second;

            for(const string& sensor_name : current_incubator.sensors)
            {
                temp_msg.sensor_name = sensor_name;
                // every sensor in an incubator reports the same temperature
                temp_msg.value = current_incubator.temperature;
                pub_temp->publish(temp_msg);
            }
        }
    }

    void set_fan_state(const gaia_incubator::msg::FanState::SharedPtr msg)
    {
        try
        {
            incubators.at(msg->incubator_id).fans.at(msg->fan_name).on
                = msg->on;
        }
        catch(const out_of_range) {} // TODO: log error with ROS
    }

    void add_incubator(const gaia_incubator::msg::AddIncubator::SharedPtr msg)
    {
        incubator inc;
        inc.name = msg->name;
        inc.temperature = msg->temperature;

        incubators[msg->incubator_id] = inc;
    }

    void add_sensor(const gaia_incubator::msg::AddSensor::SharedPtr msg)
    {
        try
        {
            incubators.at(msg->incubator_id).sensors.insert(msg->sensor_name);
        }
        catch(const out_of_range) {} // TODO: log error with ROS
    }

    void add_fan(const gaia_incubator::msg::AddFan::SharedPtr msg)
    {
        try
        {
            fan f;
            incubators.at(msg->incubator_id).fans[msg->fan_name] = f;
        }
        catch(const out_of_range) {} // TODO: log error with ROS
    }

    Publisher<gaia_incubator::msg::Temp>::SharedPtr pub_temp;

    Subscription<gaia_incubator::msg::FanState>::SharedPtr sub_fan_state;
    Subscription<gaia_incubator::msg::AddIncubator>::SharedPtr
        sub_add_incubator;
    Subscription<gaia_incubator::msg::AddSensor>::SharedPtr sub_add_sensor;
    Subscription<gaia_incubator::msg::AddFan>::SharedPtr sub_add_fan;

    TimerBase::SharedPtr sensor_reading_timer;
    TimerBase::SharedPtr update_state_timer;

    struct fan
    {
        float speed = 0.0;
        bool on = false;
    };

    struct incubator
    {
        string name;
        float temperature;

        set<string> sensors; // sensors are tracked by name only
        map<string, fan> fans; // fans are identified by their names
    };

    map<ulong, incubator> incubators; // incubators are identified by ID
};

int main(int argc, char* argv[])
{
    cout << "starting." << endl;
    init(argc, argv);
    spin(make_shared<TempSensor>());

    shutdown();
    cout << "shut down." << endl;
    return 0;
}
