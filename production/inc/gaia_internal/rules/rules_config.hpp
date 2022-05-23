////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <cpptoml.h>

namespace gaia
{
namespace rules
{

/**
 * Initializes the rules engine with configuration options specified in a
 * user-supplied gaia.conf file.
 */
void initialize_rules_engine(std::shared_ptr<cpptoml::table>& rules_config);

} // namespace rules
} // namespace gaia
