#include <chrono>
#include <functional>
#include <memory>
#include <iostream>

#include "rclcpp/rclcpp.hpp"
#include "gaia_incubator/msg/temp.hpp"
#include "gaia_incubator/msg/fan_state.hpp"

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

        // take a temperature reading once per second
        reading_timer = this->create_wall_timer(
            1s, bind(&TempSensor::publish_temp, this));
        // recalculate the temperature state ever 100ms
        // if this interval changes, the values in calc_new_temp()
        // need to be adjusted too
        loop_timer = this->create_wall_timer(
            100ms, bind(&TempSensor::calc_new_temp, this));
    }

private:
    void calc_new_temp() {
        if(fan_state)
            fan_speed = min(3500.0, fan_speed + 50.0);
        else
            fan_speed = max(0.0, fan_speed - 50.0);

        if (fan_speed < 1000)
            curr_temp += 0.01;
        else if (fan_speed < 3000)
            curr_temp -= 0.01;
        else
            curr_temp -= 0.02;
    }

    void publish_temp()
    {
        auto temp_msg = gaia_incubator::msg::Temp();
        temp_msg.value = curr_temp;
        pub_temp->publish(temp_msg);
    }

    void set_fan_state(const gaia_incubator::msg::FanState::SharedPtr msg)
    {
        fan_state = msg->state;
    }

    Publisher<gaia_incubator::msg::Temp>::SharedPtr pub_temp;
    Subscription<gaia_incubator::msg::FanState>::SharedPtr sub_fan_state;
    TimerBase::SharedPtr reading_timer;
    TimerBase::SharedPtr loop_timer;

    double curr_temp = 85.0;
    double fan_speed = 0.0;
    bool fan_state = false;
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
