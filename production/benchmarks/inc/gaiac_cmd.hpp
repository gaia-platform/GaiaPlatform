/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

namespace gaia
{
namespace cmd
{

static constexpr char c_gaiac_folder_name[] = "catalog/gaiac";
static constexpr char c_gaiac_exec_name[] = "gaiac";

class gaiac_t
{
public:
    explicit gaiac_t(std::string gaiac_path)
        : m_gaiac_path(std::move(gaiac_path)){};

    gaiac_t()
        : gaiac_t(find_gaiac_path()){};

    void load_ddl(std::string ddl_file_name);

private:
    static std::string find_gaiac_path();

private:
    std::string m_gaiac_path;
};

} // namespace cmd
} // namespace gaia
