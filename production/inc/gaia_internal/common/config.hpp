/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <filesystem>
#include <fstream>
#include <string>

#include "cpptoml.h"

namespace gaia
{
namespace common
{

// Location where the configuration file will be copied during an SDK installation.
constexpr char c_default_conf_directory[] = "/opt/gaia/etc";

constexpr char c_default_conf_file_name[] = "gaia.conf";

constexpr char c_default_logger_conf_file_name[] = "gaia_log.conf";

// Used to look up the database data directory in the configuration file.
constexpr char c_data_dir_string_key[] = "Database.data_dir";

std::string get_conf_file_path(const char* user_file_path, const char* default_filename);

class config_t
{
    config_t(std::string config_path, bool search_default = true);

    template <class T_value>
    std::optional<T_value> get_value()
    {
    }

private:
    std::shared_ptr<cpptoml::table> root_config;
};

} // namespace common
} // namespace gaia
