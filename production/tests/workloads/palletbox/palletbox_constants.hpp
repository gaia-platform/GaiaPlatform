////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <unordered_map>

namespace gaia
{
namespace palletbox
{

// Constants for stations.
enum class station_id_t
{
    none,
    charging,
    outbound,
    inbound
};

static constexpr char c_sandbox_station_charging[] = "charging";
static constexpr char c_sandbox_station_outbound[] = "outbound";
static constexpr char c_sandbox_station_inbound[] = "inbound";

extern std::unordered_map<int, const char*> g_station_name_map;

} // namespace palletbox
} // namespace gaia
