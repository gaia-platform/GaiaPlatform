////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

namespace gaia
{
namespace amr_swarm
{

enum class station_types
{
    Charging,
    Pallet,
    Widget
};

enum class robot_types
{
    Pallet,
    Widget
};

static constexpr int c_robot_id_none = 0;
static constexpr char c_pallet_id_none[] = "";

enum class stations
{
    None,
    Charging,
    Outbound,
    Inbound,
    Pl_start,
    Pl_end,
    Buffer,
    Pl_area
};

} // namespace amr_swarm
} // namespace gaia
