/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/gaiat/catalog_facade.hpp"

namespace gaia
{
namespace catalog
{
namespace gaiat
{

//
// table_facade_t
//

std::string table_facade_t::class_name(const std::string& table_name)
{
    return table_name + catalog::generate::c_class_name_suffix;
}

} // namespace gaiat
} // namespace catalog
} // namespace gaia
