/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "bot_helpers.hpp"

#include <chrono>

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::direct_access;
using namespace gaia::amr_swarm;
using namespace gaia::amr_swarm::exceptions;
using namespace gaia::rules;

using namespace exceptions;
using namespace std::chrono;

using std::string;
using std::vector;

namespace gaia
{
namespace amr_swarm
{
namespace helpers
{

uint64_t get_time_millis()
{
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

static uint global_robot_id = c_robot_id_none + 1;

static constexpr char c_sandbox_station_type_charging[] = "charging_station";
static constexpr char c_sandbox_station_type_pallet[] = "pallet_area";
static constexpr char c_sandbox_station_type_widget[] = "widget_area";

std::unordered_map<const station_types, const char*> station_type_map = {
    {station_types::Charging, c_sandbox_station_type_charging},
    {station_types::Pallet, c_sandbox_station_type_pallet},
    {station_types::Widget, c_sandbox_station_type_widget}};

static constexpr char c_sandbox_robot_type_pallet[] = "pallet_bot";
static constexpr char c_sandbox_robot_type_widget[] = "widget_bot";

std::unordered_map<const robot_types, const char*> robot_type_map = {
    {robot_types::Pallet, c_sandbox_robot_type_pallet},
    {robot_types::Widget, c_sandbox_robot_type_widget}};

static constexpr char c_sandbox_station_charging[] = "charging";
static constexpr char c_sandbox_station_outbound[] = "outbound";
static constexpr char c_sandbox_station_inbound[] = "inbound";
static constexpr char c_sandbox_station_pl_start[] = "pl_start";
static constexpr char c_sandbox_station_pl_end[] = "pl_end";
static constexpr char c_sandbox_station_buffer[] = "buffer";
static constexpr char c_sandbox_station_pl_area[] = "pl_area";

std::unordered_map<const stations, const char*> station_name_map = {
    {stations::Charging, c_sandbox_station_charging},
    {stations::Outbound, c_sandbox_station_outbound},
    {stations::Inbound, c_sandbox_station_inbound},
    {stations::Pl_start, c_sandbox_station_pl_start},
    {stations::Pl_end, c_sandbox_station_pl_end},
    {stations::Buffer, c_sandbox_station_buffer},
    {stations::Pl_area, c_sandbox_station_pl_area}};

station_types get_station_type_from_sandbox_id(const string& sandbox_id)
{
    for (const auto& row : station_type_map)
    {
        if (std::strcmp(sandbox_id.c_str(), row.second) == 0)
        {
            return row.first;
        }
    }
    throw exceptions::amr_exception(gaia_fmt::format("Cannot find station type with sandbox id {}", sandbox_id));
}

robot_types get_robot_type_from_sandbox_id(const string& sandbox_id)
{
    for (const auto& row : robot_type_map)
    {
        if (std::strcmp(sandbox_id.c_str(), row.second) == 0)
        {
            return row.first;
        }
    }
    throw exceptions::amr_exception(gaia_fmt::format("Cannot find robot type with sandbox id {}", sandbox_id));
}

stations get_station_id_from_standbox_id(const string& sandbox_id)
{
    for (const auto& row : station_name_map)
    {
        if (std::strcmp(sandbox_id.c_str(), row.second) == 0)
        {
            return row.first;
        }
    }
    throw exceptions::amr_exception(gaia_fmt::format("Cannot find station with sandbox id {}", sandbox_id));
}

template <class T_table>
void delete_all_rows()
{
    for (auto obj = *T_table::list().begin();
         obj; obj = *T_table::list().begin())
    {
        obj.delete_row();
    }
}

void clear_all_tables()
{
    for (auto station_type = *station_type_t::list().begin();
         station_type; station_type = *station_type_t::list().begin())
    {
        station_type.stations().clear();
        station_type.delete_row();
    }

    for (auto station = *station_t::list().begin();
         station; station = *station_t::list().begin())
    {
        station.widgets().clear();
        station.pallets().clear();
        station.robots().clear();
        station.bot_arrived_events().clear();
        station.bot_arrived_at_charging_station_events().clear();
        station.bot_arrived_at_inbound_station_events().clear();
        station.bot_arrived_at_buffer_station_events().clear();
        station.bot_arrived_at_pl_start_station_events().clear();
        station.bot_arrived_at_pl_end_station_events().clear();
        station.bot_arrived_at_outbound_station_events().clear();
        station.bot_moving_to_station_events().clear();
        station.check_buffer_start_tasks().clear();
        station.check_production_start_tasks().clear();
        station.delete_row();
    }

    for (auto configuration = *configuration_t::list().begin();
         configuration; configuration = *configuration_t::list().begin())
    {
        configuration.robots().clear();
        configuration.main_pallet_bot().disconnect();
        configuration.left_widget_bot().disconnect();
        configuration.right_widget_bot().disconnect();

        configuration.delete_row();
    }

    for (auto robot_type = *robot_type_t::list().begin();
         robot_type; robot_type = *robot_type_t::list().begin())
    {
        robot_type.robots().clear();
        robot_type.delete_row();
    }

    for (auto robot = *robot_t::list().begin();
         robot; robot = *robot_t::list().begin())
    {
        robot.widgets().clear();
        robot.pallets().clear();

        robot.bot_arrived_events().clear();
        robot.bot_arrived_at_charging_station_events().clear();
        robot.bot_arrived_at_inbound_station_events().clear();
        robot.bot_arrived_at_buffer_station_events().clear();
        robot.bot_arrived_at_pl_start_station_events().clear();
        robot.bot_arrived_at_pl_end_station_events().clear();
        robot.bot_arrived_at_outbound_station_events().clear();

        robot.bot_picked_up_payload_events().clear();
        robot.bot_dropped_off_payload_events().clear();
        robot.bot_cannot_navigate_events().clear();
        robot.bot_crashed_events().clear();
        robot.bot_charging_events().clear();
        robot.bot_is_charged_events().clear();
        robot.bot_moving_to_station_events().clear();
        robot.bot_out_of_battery_events().clear();
        robot.bot_battery_charge_update_events().clear();

        robot.delete_row();
    }

    for (auto pallet = *pallet_t::list().begin();
         pallet; pallet = *pallet_t::list().begin())
    {
        pallet.widgets().clear();

        pallet.pallet_arrived_events().clear();
        pallet.pallet_shipped_events().clear();
        pallet.pallet_unpacked_events().clear();

        pallet.delete_row();
    }

    for (auto widget = *widget_t::list().begin();
         widget; widget = *widget_t::list().begin())
    {
        widget.widget_ready_for_production_line_event().clear();
        widget.widget_unloaded_from_production_line_event().clear();
        widget.widget_production_finished_event().clear();

        widget.delete_row();
    }

    // Keep these in the same order as the DDL for easier reading.
    delete_all_rows<setup_started_event_t>();
    delete_all_rows<setup_complete_event_t>();

    delete_all_rows<bot_arrived_event_t>();
    delete_all_rows<bot_arrived_at_charging_station_event_t>();
    delete_all_rows<bot_arrived_at_inbound_station_event_t>();
    delete_all_rows<bot_arrived_at_buffer_station_event_t>();
    delete_all_rows<bot_arrived_at_pl_start_station_event_t>();
    delete_all_rows<bot_arrived_at_pl_end_station_event_t>();
    delete_all_rows<bot_arrived_at_outbound_station_event_t>();

    delete_all_rows<bot_moving_to_station_event_t>();
    delete_all_rows<bot_picked_up_payload_event_t>();
    delete_all_rows<bot_dropped_off_payload_event_t>();
    delete_all_rows<bot_cannot_navigate_event_t>();
    delete_all_rows<bot_crashed_event_t>();
    delete_all_rows<bot_charging_event_t>();
    delete_all_rows<bot_is_charged_event_t>();
    delete_all_rows<bot_battery_charge_update_event_t>();
    delete_all_rows<bot_out_of_battery_event_t>();

    delete_all_rows<pallet_arrived_event_t>();
    delete_all_rows<pallet_unpacked_event_t>();
    delete_all_rows<pallet_shipped_event_t>();

    delete_all_rows<widget_unloaded_from_production_line_event_t>();
    delete_all_rows<widget_ready_for_production_line_event_t>();
    delete_all_rows<widget_production_finished_event_t>();

    delete_all_rows<delayed_event_t>();
    delete_all_rows<requeue_task_t>();

    delete_all_rows<check_buffer_start_task_t>();
    delete_all_rows<check_production_start_task_t>();

    delete_all_rows<move_pallet_bot_to_inbound_task_t>();
    delete_all_rows<move_widget_bot_to_line_end_task_t>();
    delete_all_rows<move_widget_bot_to_buffer_task_t>();
    delete_all_rows<move_widget_bot_to_outbound_task_t>();
    delete_all_rows<check_for_robot_starts_task_t>();
    delete_all_rows<check_for_outbound_ship_task_t>();
}

gaia::direct_access::edc_iterator_t<gaia::amr_swarm::station_t> find_station_by_id(const stations station_id_to_match)
{
    auto station_iter = station_t::list().where(station_t::expr::id == (int)station_id_to_match).begin();
    if (station_iter == station_t::list().end())
    {
        gaia_log::app().error("Could not find station with id:{}", station_id_to_match);
        return station_t::list().end();
    }
    return station_iter;
}

gaia::direct_access::edc_iterator_t<gaia::amr_swarm::station_t> find_station_by_sandbox_id(const string& station_id_to_match)
{
    auto station_iter = station_t::list().where(station_t::expr::sandbox_id == station_id_to_match).begin();
    if (station_iter == station_t::list().end())
    {
        gaia_log::app().error("Could not find station with id:{}", station_id_to_match);
        return station_t::list().end();
    }
    return station_iter;
}

gaia::direct_access::edc_iterator_t<gaia::amr_swarm::pallet_t> find_pallet_by_id(const string& pallet_id_to_match)
{
    auto pallet_iter = pallet_t::list().where(pallet_t::expr::id == pallet_id_to_match).begin();
    if (pallet_iter == pallet_t::list().end())
    {
        gaia_log::app().error("Could not find pallet with id:{}", pallet_id_to_match);
        return pallet_t::list().end();
    }
    return pallet_iter;
}

gaia::direct_access::edc_iterator_t<gaia::amr_swarm::robot_t> find_robot_by_sandbox_id(const string& robot_sandbox_id_to_match)
{
    auto robot_iter = robot_t::list().where(robot_t::expr::sandbox_id == robot_sandbox_id_to_match).begin();
    if (robot_iter == robot_t::list().end())
    {
        gaia_log::app().error("Could not find robot with id:{}", robot_sandbox_id_to_match);
        return robot_t::list().end();
    }
    return robot_iter;
}

gaia::direct_access::edc_iterator_t<gaia::amr_swarm::widget_t> find_widget_by_id(const string& widget_id_to_match)
{
    auto widget_iter = widget_t::list().where(widget_t::expr::id == widget_id_to_match).begin();
    if (widget_iter == widget_t::list().end())
    {
        gaia_log::app().error("Could not find widget with id:{}", widget_id_to_match);
        return widget_t::list().end();
    }
    return widget_iter;
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
    pallet_w.max_widgets = 4;
    return pallet_t::get(pallet_w.insert_row());
}

void add_pallet(const string& station_id, const string& payload)
{
    auto station_iter = find_station_by_sandbox_id(station_id);
    if (station_iter == station_t::list().end())
    {
        return;
    }

    pallet_t pallet;

    auto pallet_iter = station_iter->pallets().begin();
    if (pallet_iter != station_iter->pallets().end())
    {
        auto pallet_writer = pallet_iter->writer();
        pallet_writer.station_id = (int)stations::None;
        pallet_writer.update_row();
    }

    auto pallet_writer = pallet.writer();
    pallet_writer.station_id = station_iter->id();
    pallet_writer.update_row();

    pallet_arrived_event_t::insert_row(get_time_millis(), pallet.id());
}

void dump_station(const station_t& s)
{
    printf("station id:         %s(%d)\n", s.sandbox_id(), s.id());
    printf("station_type:       %s(%d)\n", s.type().sandbox_id(), s.type().id());
}

void dump_robot_type(const robot_type_t& rt)
{
    printf("robot type id:      %s(%d)\n", rt.sandbox_id(), rt.id());
    printf("pallet_capacity:    %d\n", rt.pallet_capacity());
    printf("widget_capacity:    %d\n", rt.widget_capacity());
}

void dump_robot(const robot_t& r)
{
    printf("robot id:           %s(%d)\n", r.sandbox_id(), r.id());
    printf("robot is_idle:      %d\n", r.is_idle());
    printf("type:               %s(%d)\n", r.type().sandbox_id(), r.type().id());
    if (r.station())
    {
        printf("        station:    %s(%d)\n", r.station().sandbox_id(), r.station().id());
    }
}

void dump_pallet(const pallet_t& p)
{
    printf("pallet id:          %s\n", p.id());
    if (p.robot())
    {
        printf("on robot:           %s(%d)\n", p.robot().sandbox_id(), p.robot().id());
    }
    if (p.station())
    {
        printf("at station:         %s(%d)\n", p.station().sandbox_id(), p.station().id());
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
        printf("on robot:           %s(%d)\n", w.robot().sandbox_id(), w.robot().id());
    }
    if (w.station())
    {
        printf("at station:         %s(%d)\n", w.station().sandbox_id(), w.station().id());
    }
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

} // namespace helpers
} // namespace amr_swarm
} // namespace gaia
