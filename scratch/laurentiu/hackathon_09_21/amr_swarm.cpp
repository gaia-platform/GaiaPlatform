/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "gaia/logger.hpp"
#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"

#include "communication.hpp"
#include "constants.hpp"
#include "exceptions.hpp"
#include "gaia_amr_swarm.h"
#include "json.hpp"

using json = nlohmann::json;
using std::string;
using std::vector;

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::direct_access;
using namespace gaia::amr_swarm;
using namespace gaia::amr_swarm::exceptions;
using namespace gaia::rules;

using namespace std::chrono;

namespace gaia
{
namespace amr_swarm
{

void dump_robot(const robot_t& r);
void dump_db();

uint64_t get_time_millis()
{
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

template <class T_table>
void delete_all_rows()
{
    for (auto obj = *T_table::list().begin();
        obj;
        obj = *T_table::list().begin())
    {
        obj.delete_row();
    }
}

void clear_all_tables()
{
    for (auto station_type = *station_type_t::list().begin();
        station_type;
        station_type = *station_type_t::list().begin())
    {
        station_type.stations().clear();
        station_type.delete_row();
    }

    for (auto station = *station_t::list().begin();
        station;
        station = *station_t::list().begin())
    {
        station.robots().clear();
        station.pallets().clear();
        station.widgets().clear();
        station.bot_moving_to_station_events().clear();
        station.bot_arrived_at_station_events().clear();
        station.delete_row();
    }

    for (auto robot_type = *robot_type_t::list().begin();
        robot_type;
        robot_type = *robot_type_t::list().begin())
    {
        robot_type.robots().clear();
        robot_type.delete_row();
    }

    for (auto robot = *robot_t::list().begin();
        robot;
        robot = *robot_t::list().begin())
    {
        robot.pallets().clear();
        robot.widgets().clear();
        robot.bot_moving_to_station_events().clear();
        robot.bot_arrived_at_station_events().clear();
        robot.bot_pick_up_payload_events().clear();
        robot.bot_drop_off_payload_events().clear();
        robot.bot_charging_events().clear();
        robot.bot_cant_navigate_events().clear();
        robot.bot_crashed_events().clear();
        robot.delete_row();
    }

    for (auto pallet = *pallet_t::list().begin();
        pallet;
        pallet = *pallet_t::list().begin())
    {
        pallet.widgets().clear();
        pallet.pallet_arrived_events().clear();
        pallet.pallet_unpacked_event().disconnect();
        pallet.delete_row();
    }

    for (auto widget = *widget_t::list().begin();
        widget;
        widget = *widget_t::list().begin())
    {
        widget.widget_production_start_event().disconnect();
        widget.widget_production_end_event().disconnect();
        widget.widget_processed_event().disconnect();
        widget.delete_row();
    }

    delete_all_rows<ping_event_t>();

    delete_all_rows<pallet_arrived_event_t>();
    delete_all_rows<pallet_unpacked_event_t>();

    delete_all_rows<bot_moving_to_station_event_t>();
    delete_all_rows<bot_arrived_at_station_event_t>();
    delete_all_rows<bot_pick_up_payload_event_t>();
    delete_all_rows<bot_drop_off_payload_event_t>();
    delete_all_rows<bot_charging_event_t>();
    delete_all_rows<bot_cant_navigate_event_t>();
    delete_all_rows<bot_crashed_event_t>();

    delete_all_rows<widget_production_start_event_t>();
    delete_all_rows<widget_production_end_event_t>();
    delete_all_rows<widget_processed_event_t>();
}

robot_t add_robot(const json& j)
{
    auto robot_w = robot_writer();
    robot_w.id = j["id"].get<string>();
    robot_w.charge = 100;
    robot_w.is_stuck = false;
    robot_w.is_idle = true;
    robot_w.is_loaded = false;
    return robot_t::get(robot_w.insert_row());
}

station_t add_station(const json& j)
{
    station_t new_station;
    auto station_w = station_writer();
    station_w.id = j["id"].get<string>();
    new_station = station_t::get(station_w.insert_row());

    auto jbots = j["bots"];

    for (auto rt_iter = robot_type_t::list().begin();
        rt_iter != robot_type_t::list().end();
        ++rt_iter)
    {
        string rt_id = rt_iter->id();
        if (jbots.contains(rt_id))
        {
            for (auto& it : jbots[rt_id])
            {
                robot_t robot = add_robot(it);
                rt_iter->robots().insert(robot);
                new_station.robots().insert(robot);
            }
        }
    }
    return new_station;
}

station_type_t add_station_type(const json& j)
{
    auto station_type_w = station_type_writer();
    station_type_w.id = j["id"].get<string>();
    station_type_w.pallet_capacity = j["pallet_capacity"].get<uint32_t>();
    station_type_w.widget_capacity = j["widget_capacity"].get<uint32_t>();
    station_type_t new_station_type = station_type_t::get(station_type_w.insert_row());

    json jareas = j["areas"];

    if (jareas.is_array())
    {
        for (auto& jarea : jareas)
        {
            station_t station = add_station(jarea);
            new_station_type.stations().insert(station);
        }
    }

    return new_station_type;
}

robot_type_t add_robot_type(const json& j)
{
    auto robot_type_w = robot_type_writer();
    robot_type_w.id = j["id"].get<string>();
    robot_type_w.pallet_capacity = j["pallet_capacity"].get<uint32_t>();
    robot_type_w.widget_capacity = j["widget_capacity"].get<uint32_t>();
    return robot_type_t::get(robot_type_w.insert_row());
}

widget_t add_widget(const string& id)
{
    auto widget_w = widget_writer();
    widget_w.id = id;
    return widget_t::get(widget_w.insert_row());
}

pallet_t add_pallet(const string& id)
{
    auto pallet_w = pallet_writer();
    pallet_w.id = id;
    return pallet_t::get(pallet_w.insert_row());
}

void add_pallet(const string& station_id, const string& payload)
{
    auto station_iter = station_t::list().where(station_t::expr::id == station_id).begin();
    if (station_iter == station_t::list().end())
    {
        return;
    }

    auto j = json::parse(payload);
    pallet_t pallet = add_pallet(j["id"]);
    json jw = j["widgets"];

    if (jw.is_array())
    {
        for (auto& it : jw)
        {
            widget_t w = add_widget(it["id"]);
            pallet.widgets().insert(w);
        }
    }

    auto pallet_arrived_event = pallet_arrived_event_t::insert_row(get_time_millis());
    station_iter->pallets().insert(pallet);
    pallet.pallet_arrived_events().connect(pallet_arrived_event);
}

void handle_factory_data(const json& j)
{
    clear_all_tables();

    auto jrobot_types = j["robot_types"];

    if (jrobot_types.is_array())
    {
        for (auto& jrobot_type : jrobot_types)
        {
            add_robot_type(jrobot_type);
        }
    }

    auto jareas = j["areas_by_type"];

    for (json::iterator it = jareas.begin(); it != jareas.end(); ++it)
    {
        add_station_type(it.value());
    }
}

void log_event(const string& action_str, const string& payload)
{
    if (action_str == "factory_data")
    {
        handle_factory_data(json::parse(payload));
        dump_db();
        gaia_log::app().info("Successfully connected to the Sandbox!");
    }
    else if (action_str == "ping")
    {
        ping_event_t::insert_row(get_time_millis());
    }
    else if (action_str == "unpacked_pallet")
    {
        auto pallet_iter = pallet_t::list().where(pallet_t::expr::id == payload).begin();
        if (pallet_iter == pallet_t::list().end())
        {
            gaia_log::app().error("Could not find pallet with id:{}", payload);
            return;
        }

        auto pallet_unpacked_event = pallet_unpacked_event_t::insert_row(get_time_millis());
        pallet_iter->pallet_unpacked_event().connect(pallet_unpacked_event);
    }
    else if (action_str == "production_start_ready")
    {
        auto widget_iter = widget_t::list().where(widget_t::expr::id == payload).begin();
        if (widget_iter == widget_t::list().end())
        {
            gaia_log::app().error("Could not find widget with id:{}", payload);
            return;
        }

        auto widget_production_start_event = widget_production_start_event_t::insert_row(get_time_millis());
        widget_iter->widget_production_start_event().connect(widget_production_start_event);
    }
    else if (action_str == "production_finished")
    {
        auto widget_iter = widget_t::list().where(widget_t::expr::id == payload).begin();
        if (widget_iter == widget_t::list().end())
        {
            gaia_log::app().error("Could not find widget with id:{}", payload);
            return;
        }

        auto widget_production_end_event = widget_production_end_event_t::insert_row(get_time_millis());
        widget_iter->widget_production_end_event().connect(widget_production_end_event);
    }
    else if (action_str == "processed_widget")
    {
        auto widget_iter = widget_t::list().where(widget_t::expr::id == payload).begin();
        if (widget_iter == widget_t::list().end())
        {
            gaia_log::app().error("Could not find widget with id:{}", payload);
            return;
        }

        auto widget_processed_event = widget_processed_event_t::insert_row(get_time_millis());
        widget_iter->widget_processed_event().connect(widget_processed_event);
    }
}

void log_bot_event(const string& bot_id, const string& action_str, const string& payload)
{
    auto bot_iter = robot_t::list().where(robot_t::expr::id == bot_id).begin();
    if (bot_iter == robot_t::list().end())
    {
        gaia_log::app().error("Could not find bot with id:{}", payload);
        return;
    }

    if (action_str == "moving_to")
    {
        auto station_iter = station_t::list().where(station_t::expr::id == payload).begin();
        if (station_iter == station_t::list().end())
        {
            gaia_log::app().error("Could not find station with id:{}", payload);
            return;
        }

        auto bot_moving_to_station_event = bot_moving_to_station_event_t::insert_row(get_time_millis());
        bot_iter->bot_moving_to_station_events().connect(bot_moving_to_station_event);
        station_iter->bot_moving_to_station_events().connect(bot_moving_to_station_event);
    }
    else if (action_str == "arrived")
    {
        auto station_iter = station_t::list().where(station_t::expr::id == payload).begin();
        if (station_iter == station_t::list().end())
        {
            gaia_log::app().error("Could not find station with id:{}", payload);
            return;
        }

        // Non charging station can have at most 1 robot.
        if (strcmp(station_iter->id(), c_station_charging) != 0 && station_iter->robots().size() > 0)
        {
            gaia_log::app().error("The robot:{} cannot use station:{} because it already has a robot.", bot_iter->id(), payload);
            throw amr_exception("Failed payload_picked_up");
        }

        auto bot_arrived_at_station_event = bot_arrived_at_station_event_t::insert_row(get_time_millis());
        bot_iter->bot_arrived_at_station_events().connect(bot_arrived_at_station_event);
        station_iter->bot_arrived_at_station_events().connect(bot_arrived_at_station_event);
    }
    else if (action_str == "payload_picked_up")
    {
        if (!bot_iter->station())
        {
            gaia_log::app().error("The bot {} must be at a station to pick up a payload.", bot_iter->id());
            throw amr_exception("Failed payload_picked_up");
        }

        auto bot_pick_up_payload_event = bot_pick_up_payload_event_t::insert_row(get_time_millis());
        bot_iter->bot_pick_up_payload_events().connect(bot_pick_up_payload_event);
    }
    else if (action_str == "payload_dropped")
    {
        if (!bot_iter->station())
        {
            gaia_log::app().error("The bot {} must be at a station to drop off a payload.", bot_iter->id());
            throw amr_exception("Failed payload_dropped");
        }

        auto bot_drop_off_payload_event = bot_drop_off_payload_event_t::insert_row(get_time_millis());
        bot_iter->bot_drop_off_payload_events().connect(bot_drop_off_payload_event);
    }
    else if (action_str == "charging")
    {
        if (!bot_iter->station())
        {
            gaia_log::app().error("The bot {} must be at a station to be charged.", bot_iter->id());
            throw amr_exception("Failed charging");
        }

        auto bot_charging_event = bot_charging_event_t::insert_row(get_time_millis());
        bot_iter->bot_charging_events().connect(bot_charging_event);
    }
    else if (action_str == "cant_navigate")
    {
        auto bot_cant_navigate_event = bot_cant_navigate_event_t::insert_row(get_time_millis());
        bot_iter->bot_cant_navigate_events().connect(bot_cant_navigate_event);
    }
    else if (action_str == "crashed")
    {
        auto bot_crashed_event = bot_crashed_event_t::insert_row(get_time_millis());
        bot_iter->bot_crashed_events().connect(bot_crashed_event);
    }
    else
    {
        throw amr_exception("Unexpected bot action: " + action_str);
    }
}

vector<string> split_topic(const string& topic)
{
    vector<string> result;
    size_t left = 0;
    size_t right = topic.find('/');
    while (right != string::npos)
    {
        result.push_back(topic.substr(left, right - left));
        left = right + 1;
        right = topic.find('/', left);
    }
    result.push_back(topic.substr(left));
    return result;
}

void on_message(const string& topic, const string& payload)
{
    gaia_log::app().info("Message received topic:{} payload:{}", topic.c_str(), payload.c_str());

    vector<string> topic_vector = split_topic(topic.c_str());

    if (topic_vector.size() < 2)
    {
        gaia_log::app().error("Unexpected topic:{}", topic.c_str());
        return;
    }

    begin_transaction();
    if (topic_vector.size() >= 4)
    {
        if (topic_vector[1] == "bot")
        {
            log_bot_event(topic_vector[2], topic_vector[3], payload);
        }
        else if (topic_vector[1] == "station" && topic_vector[3] == "pallet")
        {
            add_pallet(topic_vector[2], payload);
        }
    }
    else
    {
        log_event(topic_vector[1], payload);
    }
    commit_transaction();
}

void dump_station(const station_t& s)
{
    printf("station id:         %s\n", s.id());
    printf("station_type:       %s\n", s.type().id());
}

void dump_robot_type(const robot_type_t& rt)
{
    printf("robot type id:      %s\n", rt.id());
    printf("pallet_capacity:    %d\n", rt.pallet_capacity());
    printf("widget_capacity:    %d\n", rt.widget_capacity());
}

void dump_robot(const robot_t& r)
{
    printf("robot id:           %s\n", r.id());
    printf("type:               %s\n", r.type().id());
    if (r.station())
    {
        printf("        station:    %s\n", r.station().id());
    }
}

void dump_pallet(const pallet_t& p)
{
    printf("pallet id:          %s\n", p.id());
    if (p.robot())
    {
        printf("on robot:           %s\n", p.robot().id());
    }
    if (p.station())
    {
        printf("at station:         %s\n", p.station().id());
    }
}

void dump_widget(const widget_t& w)
{
    printf("widget id:          %s\n", w.id());
    if (w.pallet())
    {
        printf("on pallet:          %s\n", w.pallet().id());
    }
    if (w.robot())
    {
        printf("on robot:           %s\n", w.robot().id());
    }
    if (w.station())
    {
        printf("at station:         %s\n", w.station().id());
    }
}

void dump_db_state()
{
    printf("--------------------------------------------------------\n");
    for (const auto& r : robot_t::list())
    {
        printf("--------------------------------------------------------\n");
        dump_robot(r);
    }
    printf("--------------------------------------------------------\n");
    for (const auto& p : pallet_t::list())
    {
        printf("--------------------------------------------------------\n");
        dump_pallet(p);
    }
    printf("--------------------------------------------------------\n");
    for (const auto& w : widget_t::list())
    {
        printf("--------------------------------------------------------\n");
        dump_widget(w);
    }
    printf("--------------------------------------------------------\n");
}

void dump_db()
{
    printf("\n");
    for (const auto& s : station_t::list())
    {
        printf("--------------------------------------------------------\n");
        dump_station(s);
    }
    printf("--------------------------------------------------------\n");
    for (const auto& rt : robot_type_t::list())
    {
        printf("--------------------------------------------------------\n");
        dump_robot_type(rt);
    }
    printf("--------------------------------------------------------\n");
    dump_db_state();
}

} // namespace amr_swarm
} // namespace gaia

int main(int argc, char* argv[])
{
    gaia::system::initialize();

    if (!communication::init(argc, argv))
    {
        return 1;
    }

    begin_transaction();
    clear_all_tables();
    commit_transaction();
    communication::connect(on_message);
    gaia::system::shutdown();

    return 0;
}
