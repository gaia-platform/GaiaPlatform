////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "json.hpp"

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

using json_t = nlohmann::json;

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
private:
    static constexpr int c_default_json_indentation = 4;

public:
    gaia_id_t insert_station(station_id_t station_id)
    {
        station_writer station_w;
        station_w.id = static_cast<int>(station_id);
        return station_w.insert_row();
    }

    gaia_id_t insert_robot(int robot_id)
    {
        robot_writer robot_w;
        robot_w.id = robot_id;
        robot_w.times_to_charging = 0;
        robot_w.target_times_to_charge = 1;
        gaia_id_t gid = robot_w.insert_row();
        return gid;
    }

    void restore_station_table()
    {
        for (auto& station_table : station_t::list())
        {
            station_writer station_w = station_table.writer();
            station_w.update_row();
        }
    }

    void restore_robot_table(robot_t& robot_table)
    {
        robot_writer robot_w = robot_table.writer();
        robot_w.times_to_charging = 0;
        robot_w.target_times_to_charge = 0;
        robot_w.station_id = static_cast<int>(station_id_t::charging);
        robot_w.update_row();
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
        auto_transaction_t txn(auto_transaction_t::no_auto_restart);

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
        robot_writer robot_w = pallet_bot.writer();
        robot_w.station_id = static_cast<int>(station_id_t::charging);
        robot_w.update_row();
        txn.commit();
    }

    json_t to_json(const station_t& station)
    {
        json_t json;
        json["id"] = station.id();
        json["name"] = g_station_name_map[station.id()];
        return json;
    }

    static json_t to_json(const robot_t& robot)
    {
        json_t json;
        json["id"] = robot.id();
        json["station_id"] = robot.station_id();
        json["times_to_charging"] = robot.times_to_charging();
        json["target_times_to_charge"] = robot.target_times_to_charge();
        return json;
    }

    static json_t to_json(const string& property_name, int property_value)
    {
        json_t json;
        json[property_name] = property_value;
        return json;
    }

    void dump_db_json(FILE* object_log_file) override
    {
        begin_transaction();

        json_t json_document;
        for (const station_t& station : station_t::list())
        {
            json_document["stations"].push_back(to_json(station));
        }
        for (const robot_t& robot : robot_t::list())
        {
            json_document["robots"].push_back(to_json(robot));
        }
        json_document["event_counts"].push_back(to_json(
            "bot_moving_to_station_event", bot_moving_to_station_event_t::list().size()));
        json_document["event_counts"].push_back(to_json(
            "bot_arrived_event", bot_arrived_event_t::list().size()));
        json_document["event_counts"].push_back(to_json(
            "payload_pick_up_event", payload_pick_up_event_t::list().size()));
        json_document["event_counts"].push_back(to_json(
            "payload_drop_off_event", payload_drop_off_event_t::list().size()));

        fprintf(object_log_file, "%s", json_document.dump(c_default_json_indentation).c_str());
        commit_transaction();
    }

    void setup_test_data(int required_iterations) override
    {
        auto robot_it = robot_t::list().where(robot_expr::id == c_sole_robot_id).begin();
        if (robot_it == robot_t::list().end())
        {
            throw simulation_exception(gaia_fmt::format("Cannot find robot with id {}.", c_sole_robot_id));
        }

        robot_writer robot_w = robot_it->writer();
        robot_w.times_to_charging = 0;
        robot_w.target_times_to_charge = required_iterations;
        robot_w.update_row();

        bot_moving_to_station_event_t::insert_row(
            generate_unique_millisecond_timestamp(),
            static_cast<int>(station_id_t::inbound),
            robot_it->id());
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

    my_duration_in_microseconds_t perform_single_step() override
    {
        return my_duration_in_microseconds_t(0);
    }

    bool has_step_completed(int timestamp, int initial_state_tracker) override
    {
        return true;
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
