/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

#include "constants.hpp"
#include "gaia_amr_swarm.h"

namespace gaia
{
namespace amr_swarm
{
namespace commands
{

extern bool show_outgoing_robot_messages;

void move_robot_to_station(const gaia::amr_swarm::robot_waynetype& robot, const gaia::amr_swarm::station_waynetype station);
void move_robot_to_station(robot_waynetype& robot, const stations station_id);

void pickup_pallet_from_station(const gaia::amr_swarm::robot_waynetype& robot, const gaia::amr_swarm::pallet_waynetype pallet);
void pickup_widget_from_station(const gaia::amr_swarm::robot_waynetype& robot, const gaia::amr_swarm::widget_waynetype widget);
void drop_pallet_at_station(const gaia::amr_swarm::robot_waynetype& robot);
void drop_widget_at_station(const gaia::amr_swarm::robot_waynetype& robot);

void start_charging_robot(const gaia::amr_swarm::robot_waynetype& robot);

void request_charge_level(const gaia::amr_swarm::robot_waynetype& robot);

void press_unpack_pallet_button();
void press_start_production_button();
void press_unload_pl_button();
void press_ship_button();
void press_receive_order_button();

} // namespace commands
} // namespace amr_swarm
} // namespace gaia
