#include <memory>
#include <chrono>
#include <functional>

#include "gaia_incubator/gaia_logic.hpp"

int main(int argc, char* argv[])
{
    cout << "Starting gaia_logic." << endl;
    init(argc, argv);
    spin(make_shared<gaia_logic>());

    shutdown();
    cout << "Shut down gaia_logic." << endl;
    return 0;
}

gaia_logic::gaia_logic(): Node("gaia_logic")
{
    RCLCPP_INFO(get_logger(), "hello world!");
}
