////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <string>

namespace gaia
{
namespace test
{

/**
 * Finds a resource in the cmake build directory. Starts searching from the current path
 * moving to the parent folder until it finds a match.
 *
 * @param path the path from the root of the cmake build directory.
 * @return a string representing the resource.
 * @throw std::runtime_error if the resource is not found.
 */
std::string find_resource(std::string& path);

} // namespace test
} // namespace gaia
