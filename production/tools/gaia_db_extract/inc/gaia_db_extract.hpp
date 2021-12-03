/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <vector>

namespace gaia
{
namespace tools
{
namespace db_extract
{

constexpr uint64_t c_start_at_first = 0;
constexpr uint32_t c_row_limit_unlimited = -1;
constexpr char c_empty_object[] = "{}";

/**
 * Initialize the type cache before extracting row data.
 */
bool gaia_db_extract_initialize(void);

/**
 * Return a string containing the JSON representation of data contained in
 * the current gaia_db_server. The specific data is requested by the parameters.
 */
std::string gaia_db_extract(std::string database, std::string table, uint64_t start_after, uint32_t row_limit);

} // namespace db_extract
} // namespace tools
} // namespace gaia
