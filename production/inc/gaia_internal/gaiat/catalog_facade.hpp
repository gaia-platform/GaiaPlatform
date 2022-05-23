////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include "gaia_internal/catalog/catalog_facade.hpp"

/*
 * Provides gaiat specific facade.
 */

namespace gaia
{
namespace catalog
{
namespace gaiat
{

class table_facade_t : public gaia::catalog::generate::table_facade_t
{
public:
    static std::string class_name(const std::string& table_name);
};

} // namespace gaiat
} // namespace catalog
} // namespace gaia
