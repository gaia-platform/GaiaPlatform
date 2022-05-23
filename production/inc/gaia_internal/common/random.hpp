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
namespace common
{

/**
 * Generates a random integer in the given range, including low and high.
 */
int gen_random_num(int low, int high);

/**
 * Generates a random alpha-numeric string of the given length.
 */
std::string gen_random_str(size_t len);

} // namespace common
} // namespace gaia
