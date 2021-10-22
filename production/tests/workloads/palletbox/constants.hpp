
#pragma once

namespace gaia
{
namespace palletbox
{

#include <unordered_map>

enum class station_id_t {
    none,
    charging,
    outbound,
    inbound
};

extern std::unordered_map<int, const char*> station_name_map;

} // namespace palletbox
} // namespace gaia

uint64_t get_time_millis();
