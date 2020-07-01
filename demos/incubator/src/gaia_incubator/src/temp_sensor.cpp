#include <chrono>
#include <functional>
#include <memory>
#include <iostream>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/temperature.hpp"

using namespace std;
using namespace rclcpp;

class TempSensor : public Node
{
public:
    TempSensor(): Node("temp_sensor")
    {
        using namespace chrono_literals;

        pub_temp =
            this->create_publisher<sensor_msgs::msg::Temperature>("temp", 10);

        reading_timer = this->create_wall_timer(
            1s, bind(&TempSensor::publish_temp, this));
        loop_timer = this->create_wall_timer(
            100ms, bind(&TempSensor::calc_new_temp, this));
    }

private:
    void calc_new_temp() {
        if (fan_speed < 1000)
            curr_temp += 0.01;
        else if (fan_speed < 3000)
            curr_temp -= 0.01;
        else
            curr_temp -= 0.02;
    }

    void publish_temp()
    {
        auto temp_msg = sensor_msgs::msg::Temperature();
        temp_msg.temperature = curr_temp;
        pub_temp->publish(temp_msg);
    }

    Publisher<sensor_msgs::msg::Temperature>::SharedPtr pub_temp;
    TimerBase::SharedPtr reading_timer;
    TimerBase::SharedPtr loop_timer;

    double curr_temp = 85.0;
    double fan_speed = 0.0;
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
