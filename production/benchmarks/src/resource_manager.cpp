////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "resource_manager.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace gaia
{
namespace test
{

std::string find_resource(std::string& path)
{
    fs::path current_path = fs::current_path();
    fs::path target_path = fs::path(current_path) / path;

    while (!fs::exists(target_path) && current_path.has_root_path() && current_path != current_path.root_path())
    {
        current_path = current_path.parent_path();
        target_path = fs::path(current_path) / path;
    }

    if (!fs::exists(target_path))
    {
        throw std::runtime_error(std::string("Impossible to find resource: ") + path);
    }

    return target_path.string();
}

} // namespace test
} // namespace gaia
