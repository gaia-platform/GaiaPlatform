/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaiac_cmd.hpp"

#include <cstring>

#include <stdexcept>

#include "gaia_spdlog/fmt/fmt.h"

#include "resource_manager.hpp"

namespace gaia
{
namespace cmd
{

std::string gaiac_t::find_gaiac_path()
{
    std::string gaiac_path = gaia_fmt::format("{}/{}", c_gaiac_folder_name, c_gaiac_exec_name);
    return gaia::test::find_resource(gaiac_path);
}

void gaiac_t::load_ddl(std::string ddl_file_name)
{
    std::string full_ddl_path = gaia::test::find_resource(ddl_file_name);

    std::string gaiac_cmd = m_gaiac_path
        + std::string(" ")
        + full_ddl_path;

    int status = ::system(gaiac_cmd.c_str());

    if (status < 0)
    {
        throw std::runtime_error(std::string("Error: ") + std::string(strerror(errno)));
    }
}

} // namespace cmd
} // namespace gaia
