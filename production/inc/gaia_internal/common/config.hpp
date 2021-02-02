/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <fstream>
#include <string>

namespace gaia
{
namespace common
{

constexpr char c_default_conf_file_name[] = "gaia.conf";

constexpr char c_default_logger_conf_file_name[] = "gaia_log.conf";

constexpr char c_conf_file_flag[] = "--configuration-file";

std::string get_conf_file(const char* user_filename, const char* default_filename);

} // namespace common
} // namespace gaia
