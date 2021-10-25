/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_palletbox.h"
#include "palletbox_constants.hpp"
#include "simulation.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::direct_access;
using namespace gaia::simulation::exceptions;
using namespace gaia::rules;

using namespace gaia::palletbox;

typedef std::chrono::steady_clock my_clock;
typedef my_clock::time_point my_time_point;
typedef std::chrono::microseconds microseconds;
typedef std::chrono::duration<double, std::micro> my_duration_in_microseconds;

static constexpr int c_sole_robot_id = 1;

namespace gaia
{
namespace palletbox
{

std::unordered_map<int, const char*> g_station_name_map = {
    {static_cast<int>(station_id_t::charging), c_sandbox_station_charging},
    {static_cast<int>(station_id_t::outbound), c_sandbox_station_outbound},
    {static_cast<int>(station_id_t::inbound), c_sandbox_station_inbound}};

} // namespace palletbox
} // namespace gaia

class palletbox_simulation_t : public simulation_t
{
public:
    gaia_id_t insert_station(station_id_t station_id)
    {
        station_writer w;
        w.id = static_cast<int>(station_id);
        return w.insert_row();
    }

    gaia_id_t insert_robot(int robot_id)
    {
        robot_writer w;
        w.id = robot_id;
        w.times_to_charging = 0;
        w.target_times_to_charge = 1;
        gaia_id_t gid = w.insert_row();
        return gid;
    }

    void restore_station_table()
    {
        for (auto& station_table : station_t::list())
        {
            station_writer w = station_table.writer();
            w.update_row();
        }
    }

    void restore_robot_table(robot_t& robot_table)
    {
        robot_writer w = robot_table.writer();
        w.times_to_charging = 0;
        w.target_times_to_charge = 0;
        //w.station_id = (int)station_id_t::charging;
        w.update_row();
    }

    void restore_default_values()
    {
        restore_station_table();
        for (auto& robot_table : robot_t::list())
        {
            restore_robot_table(robot_table);
        }
    }

    void init_storage() override
    {
        auto_transaction_t txn(auto_transaction_t::no_auto_begin);

        // If we already have inserted a table then our storage has already been
        // initialized.  Re-initialize the database to default values.
        if (station_t::list().size())
        {
            restore_default_values();
            txn.commit();
            return;
        }

        auto charging_station = station_t::get(insert_station(station_id_t::charging));
        insert_station(station_id_t::outbound);
        insert_station(station_id_t::inbound);

        auto pallet_bot = robot_t::get(insert_robot(c_sole_robot_id));
        charging_station.robots().connect(pallet_bot);
        txn.commit();
    }

    void setup_test_data(int required_iterations) override
    {
        auto robot_it = robot_t::list().where(robot_expr::id == c_sole_robot_id).begin();
        if (robot_it == robot_t::list().end())
        {
            throw simulation_exception(gaia_fmt::format("Cannot find robot with id {}", c_sole_robot_id));
        }

        robot_writer w = robot_it->writer();
        w.times_to_charging = 0;
        w.target_times_to_charge = required_iterations;
        w.update_row();

        bot_moving_to_station_event_t::insert_row(get_time_millis(), static_cast<int>(station_id_t::inbound), robot_it->id());
    }

    bool has_test_completed() override
    {
        int times_to_charging = -1;
        int target_times_to_charge = -2;
        begin_transaction();
        for (const gaia::palletbox::robot_t& i : robot_t::list())
        {
            times_to_charging = i.times_to_charging();
            target_times_to_charge = i.target_times_to_charge();
        }
        commit_transaction();

        return times_to_charging == target_times_to_charge;
    }

    long get_processing_timeout_in_microseconds() override
    {
        return simulation_t::get_processing_timeout_in_microseconds();
    }

    string get_configuration_file_name() override
    {
        return "palletbox.conf";
    }
};

int main(int argc, const char** argv)
{
    palletbox_simulation_t this_simulation;
    return this_simulation.main(argc, argv);
}
