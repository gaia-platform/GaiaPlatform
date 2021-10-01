/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <vector>

#include "gaia/logger.hpp"
#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"

#include "constants.hpp"
#include "exceptions.hpp"
#include "gaia_amr_swarm.h"

namespace gaia
{
namespace amr_swarm
{
namespace helpers
{
uint64_t get_time_millis();

void clear_all_tables();
widget_t add_widget(const std::string& id);
pallet_t add_pallet(const std::string& id);
void add_pallet(const std::string& station_id, const std::string& payload);

gaia::direct_access::edc_iterator_t<gaia::amr_swarm::station_t> find_station_by_id(const stations station_id_to_match);
gaia::direct_access::edc_iterator_t<gaia::amr_swarm::station_t> find_station_by_sandbox_id(const std::string& station_id_to_match);
gaia::direct_access::edc_iterator_t<gaia::amr_swarm::pallet_t> find_pallet_by_id(const std::string& pallet_id_to_match);
gaia::direct_access::edc_iterator_t<gaia::amr_swarm::robot_t> find_robot_by_sandbox_id(const std::string& robot_id_to_match);
gaia::direct_access::edc_iterator_t<gaia::amr_swarm::widget_t> find_widget_by_id(const std::string& widget_id_to_match);

void dump_db();

} // namespace helpers
} // namespace amr_swarm
} // namespace gaia
