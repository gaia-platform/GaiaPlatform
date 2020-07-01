#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <iostream>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std;
using namespace rclcpp;

class TempSensor : public Node
{
public:
    TempSensor(): Node("temp_sensor")
    {
        using namespace chrono_literals;

        pubTemp = this->create_publisher<std_msgs::msg::String>("temp", 10);
        timer = this->create_wall_timer(
            1s, bind(&TempSensor::pubTemp_callback, this));
    }

private:
    void pubTemp_callback()
    {
        auto tempMsg = std_msgs::msg::String();
        tempMsg.data = "50";
        pubTemp->publish(tempMsg);
    }

    Publisher<std_msgs::msg::String>::SharedPtr pubTemp;
    TimerBase::SharedPtr timer;
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
