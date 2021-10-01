/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "bot_commands.hpp"

#include "bot_helpers.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::direct_access;
using namespace gaia::amr_swarm;
using namespace gaia::amr_swarm::exceptions;
using namespace gaia::rules;

using namespace exceptions;

using std::string;

namespace gaia
{
namespace amr_swarm
{
namespace commands
{

// Constants.
static constexpr char c_robot_topic_base[] = "bot";

// static constexpr char c_topic_suffix_move_location[] = "move_location";
// static constexpr char c_topic_suffix_status_request[] = "status_request";
// static constexpr char c_topic_suffix_pickup_payload[] = "pickup_payload";
// static constexpr char c_topic_suffix_drop_payload[] = "drop_payload";
// static constexpr char c_topic_suffix_charge[] = "charge";

// static constexpr char c_topic_unpack_pallet[] = "unpack_pallet";
// static constexpr char c_topic_start_production[] = "start_production";
// static constexpr char c_topic_unload_pl[] = "unload_pl";
// static constexpr char c_topic_ship[] = "ship";
// static constexpr char c_topic_receive_order[] = "receive_order";

// static constexpr char c_status_empty_payload[] = " ";
// static constexpr char c_status_request_payload[] = "charge_level";

// Control whether the functions are expected to output a simple log message
// detailing their request.
bool show_outgoing_robot_messages = false;

string create_bot_topic(const robot_t&, const string&)
{
    string topic(c_robot_topic_base);
    topic += "communication::c_topic_separator + string(robot.sandbox_id()) + communication::c_topic_separator + sub_topic";
    return topic;
}

void move_robot_to_station(const robot_t& robot, const gaia::amr_swarm::station_t station)
{
    if (robot.station() == station)
    {
        throw amr_exception(gaia_fmt::format("Cannot move robot {} to its same location:{}", robot.id(), station.id()));
    }

    if (show_outgoing_robot_messages)
    {
        gaia_log::app().info("BOT>>move_robot: {} to station {}.", robot.id(), station.id());
    }

    gaia_log::app().info("Requesting robot {} to transition to station {}", robot.id(), station.id());
    // communication::publish_message(create_bot_topic(robot, c_topic_suffix_move_location), station.sandbox_id());
}

void move_robot_to_station(robot_t& robot, const stations station_id)
{
    auto station_iter = station_t::list().where(station_expr::id == (int)station_id).begin();
    if (station_iter == station_t::list().end())
    {
        throw exceptions::amr_exception(gaia_fmt::format("Cannot find station with id {}", station_id));
    }

    move_robot_to_station(robot, *station_iter);
}

void request_charge_level(const robot_t& robot)
{
    if (show_outgoing_robot_messages)
    {
        gaia_log::app().info("BOT>>status_request: Requesting charge_level status for bot {}.", robot.id());
    }
    // communication::publish_message(create_bot_topic(robot, c_topic_suffix_status_request), c_status_request_payload);
}

void pickup_pallet_from_station(const robot_t& robot, const gaia::amr_swarm::pallet_t pallet)
{
    if (robot.pallets().size() > 0)
    {
        throw amr_exception(
            gaia_fmt::format("Cannot pickup pallet. The robot {} already has a payload.", robot.id()));
    }

    if (!robot.station())
    {
        throw amr_exception(
            gaia_fmt::format("Cannot pickup pallet. The robot {} is not on a station", robot.id()));
    }

    if (!pallet.station())
    {
        throw amr_exception(
            gaia_fmt::format("Cannot pickup pallet. The pallet {} is not on a station", pallet.id()));
    }

    if (robot.station() != pallet.station())
    {
        throw amr_exception(
            gaia_fmt::format(
                "Cannot pickup pallet. The robot {} is on a different station {} than the payload {}.",
                robot.id(), robot.station().id(), pallet.station().id()));
    }

    if (show_outgoing_robot_messages)
    {
        gaia_log::app().info("BOT>>pickup_pallet: from station {} with robot {}.", robot.station().id(), robot.id());
    }

    // communication::publish_message(create_bot_topic(robot, c_topic_suffix_pickup_payload), robot.station().sandbox_id());
}

void drop_pallet_at_station(const robot_t& robot)
{
    if (robot.pallets().size() == 0)
    {
        throw amr_exception(
            gaia_fmt::format("Cannot drop pallet. The robot {} has no payload.", robot.id()));
    }

    if (!robot.station())
    {
        throw amr_exception(
            gaia_fmt::format("Cannot drop pallet. The robot {} is not on a station", robot.id()));
    }

    if (show_outgoing_robot_messages)
    {
        gaia_log::app().info("BOT>>drop_pallet: robot {}.", robot.id());
    }

    // communication::publish_message(create_bot_topic(robot, c_topic_suffix_drop_payload), robot.station().sandbox_id());
}

void start_charging_robot(const robot_t& robot)
{
    if (robot.station().id() != (int)stations::Charging)
    {
        throw amr_exception(
            gaia_fmt::format("Cannot charge robot {} since it is not on a charging station waypoint", robot.id()));
    }

    if (show_outgoing_robot_messages)
    {
        gaia_log::app().info("BOT>>charge: robot {}.", robot.id());
    }

    // communication::publish_message(create_bot_topic(robot, c_topic_suffix_charge), c_status_empty_payload);
}

void drop_widget_at_station(const robot_t& robot)
{
    if (robot.widgets().size() == 0)
    {
        throw amr_exception(
            gaia_fmt::format("Cannot drop widget. The robot {} has no widget.", robot.id()));
    }

    if (!robot.station())
    {
        throw amr_exception(
            gaia_fmt::format("Cannot drop widget. The robot {} is not on a station", robot.id()));
    }

    if (show_outgoing_robot_messages)
    {
        gaia_log::app().info("BOT>>drop_widget: robot {}.", robot.id());
    }

    // communication::publish_message(create_bot_topic(robot, "drop_payload"), robot.station().sandbox_id());
}

void pickup_widget_from_station(const robot_t& robot, const gaia::amr_swarm::widget_t widget)
{
    if (robot.pallets().size() > 0)
    {
        throw amr_exception(
            gaia_fmt::format("Cannot pickup widget. The robot {} already has a payload.", robot.id()));
    }

    if (!robot.station())
    {
        throw amr_exception(
            gaia_fmt::format("Cannot pickup widget. The robot {} is not on a station", robot.id()));
    }

    if (!widget.station())
    {
        throw amr_exception(
            gaia_fmt::format("Cannot pickup widget. The widget {} is not on a station", widget.id()));
    }

    if (robot.station() != widget.station())
    {
        throw amr_exception(
            gaia_fmt::format(
                "Cannot pickup widget. The robot {} is on a different station {} than the payload {}.",
                robot.id(), robot.station().id(), widget.station().id()));
    }

    if (show_outgoing_robot_messages)
    {
        gaia_log::app().info("BOT>>pickup_widget: from station {} with robot {}.", robot.station().id(), robot.id());
    }

    // communication::publish_message(create_bot_topic(robot, c_topic_suffix_pickup_payload), robot.station().sandbox_id());
}

void press_unpack_pallet_button()
{
    if (show_outgoing_robot_messages)
    {
        gaia_log::app().info("BOT>>unpack_pallet");
    }
    // communication::publish_message(c_topic_unpack_pallet, c_status_empty_payload);
}

void press_start_production_button()
{
    if (show_outgoing_robot_messages)
    {
        gaia_log::app().info("BOT>>start_production");
    }
    // communication::publish_message(c_topic_start_production, c_status_empty_payload);
}

void press_unload_pl_button()
{
    if (show_outgoing_robot_messages)
    {
        gaia_log::app().info("BOT>>unload_pl");
    }
    // communication::publish_message(c_topic_unload_pl, c_status_empty_payload);
}

void press_ship_button()
{
    if (show_outgoing_robot_messages)
    {
        gaia_log::app().info("BOT>>ship");
    }
    // communication::publish_message(c_topic_ship, c_status_empty_payload);
}

void press_receive_order_button()
{
    if (show_outgoing_robot_messages)
    {
        gaia_log::app().info("BOT>>receive_order");
    }
    // communication::publish_message(c_topic_receive_order, c_status_empty_payload);
}

} // namespace commands
} // namespace amr_swarm
} // namespace gaia
