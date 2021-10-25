
#pragma once

#include <unordered_map>

namespace gaia
{
namespace palletbox
{

enum class station_id_t
{
    none,
    charging,
    outbound,
    inbound
};

extern std::unordered_map<int, const char*> g_station_name_map;

} // namespace palletbox
} // namespace gaia

uint64_t get_time_millis();
