#include <memory>
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
    using std::placeholders::_1;

    m_sub_temp =
        this->create_subscription<msg::Temp>(
            "temp", SystemDefaultsQoS(), std::bind(
                &gaia_logic::temp_sensor_callback, this, _1));
}

void gaia_logic::temp_sensor_callback(const msg::Temp::SharedPtr msg)
{
    cout << msg->sensor_name << ": " << msg->stamp.sec
    << " | " << msg->value << endl;
}
