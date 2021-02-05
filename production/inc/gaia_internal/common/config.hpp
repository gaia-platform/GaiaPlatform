/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <filesystem>
#include <fstream>
#include <string>

namespace gaia
{
namespace common
{

// Location where the configuration file will be copied during an SDK installation.
constexpr char c_default_conf_directory[] = "/opt/gaia/etc";

constexpr char c_default_conf_file_name[] = "gaia.conf";

constexpr char c_default_logger_conf_file_name[] = "gaia_log.conf";

constexpr char c_conf_file_flag[] = "--configuration-file-path";

// Used to look up the database data directory in the configuration file.
constexpr char c_data_dir_string_key[] = "Database.data_dir";

std::string get_conf_file_path(const char* user_file_path, const char* default_filename);

} // namespace common
} // namespace gaia
