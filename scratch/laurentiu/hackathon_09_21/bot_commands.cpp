/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "bot_commands.hpp"

#include <random>
#include <string>
#include <vector>

#include "gaia/logger.hpp"

#include "communication.hpp"
#include "constants.hpp"
#include "exceptions.hpp"

using namespace gaia::amr_swarm;
using namespace exceptions;

using std::string;

namespace gaia
{
namespace amr_swarm
{
namespace commands
{

bool show_outgoing_robot_messages = true;

void move_robot(const robot_t& robot, const gaia::amr_swarm::station_t station)
{
    if (robot.station() == station)
    {
        throw amr_exception(gaia_fmt::format("Cannot move robot {} to its same location:{}", robot.id(), station.id()));
    }

    if (show_outgoing_robot_messages)
    {
        gaia_log::app().info("BOT>>move_robot: {} to station {}.", robot.id(), station.id());
    }

    string topic("bot/");
    topic += string(robot.id()) + "/move_location";
    communication::publish_message(topic, station.id());
}

void move_robot(const robot_t& robot, const string& station_id)
{
    auto station_iter = station_t::list().where(station_expr::id == station_id).begin();

    if (station_iter == station_t::list().end())
    {
        throw amr_exception(gaia_fmt::format("Cannot find station with id {}", station_id));
    }

    move_robot(robot, *station_iter);

    while (station_iter != station_t::list().end())
    {
        ++station_iter;
    }
}

void pickup_pallet(const robot_t& robot, const gaia::amr_swarm::pallet_t pallet)
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

    string topic("bot/");
    topic += string(robot.id()) + "/pickup_payload";
    communication::publish_message(topic, robot.station().id());
}

void drop_pallet(const robot_t& robot)
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

    string topic("bot/");
    topic += string(robot.id()) + "/drop_payload";
    communication::publish_message(topic, robot.station().id());
}

void status(const robot_t& robot)
{
    // TODO you can look at the mqtt protocol. Here you send a status request and the
    //  for one of the bot's params. As a response you receive a message with the value.
    throw amr_exception(
        gaia_fmt::format("Implement it yourself!", robot.id()));
}

void charge(const robot_t& robot)
{
    if (strcmp(robot.station().id(), c_station_charging) != 0)
    {
        throw amr_exception(
            gaia_fmt::format("Cannot charge robot {} since it is not on a charging station waypoint", robot.id()));
    }

    if (show_outgoing_robot_messages)
    {
        gaia_log::app().info("BOT>>charge: robot {}.", robot.id());
    }

    string topic("bot/");
    topic += string(robot.id()) + "/charge";
    communication::publish_message(topic, " ");
}

void drop_widget(const robot_t& robot)
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

    string topic("bot/");
    topic += string(robot.id()) + "/drop_payload";
    communication::publish_message(topic, robot.station().id());
}

void pickup_widget(const robot_t& robot, const gaia::amr_swarm::widget_t widget)
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

    string topic("bot/");
    topic += string(robot.id()) + "/pickup_payload";
    communication::publish_message(topic, robot.station().id());
}

void unpack_pallet()
{
    if (show_outgoing_robot_messages)
    {
        gaia_log::app().info("BOT>>unpack_pallet");
    }
    communication::publish_message("unpack_pallet", " ");
}

void start_production()
{
    if (show_outgoing_robot_messages)
    {
        gaia_log::app().info("BOT>>start_production");
    }
    communication::publish_message("start_production", " ");
}

void unload_pl()
{
    if (show_outgoing_robot_messages)
    {
        gaia_log::app().info("BOT>>unload_pl");
    }
    communication::publish_message("unload_pl", " ");
}

void ship()
{
    if (show_outgoing_robot_messages)
    {
        gaia_log::app().info("BOT>>ship");
    }
    communication::publish_message("ship", " ");
}

} // namespace commands
} // namespace amr_swarm
} // namespace gaia
