/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/catalog/gaiat_catalog_facade.hpp"

namespace gaia
{
namespace catalog
{
namespace generate
{

//
// gaiat_incoming_link_facade_t
//

std::string gaiat_table_facade_t::class_name(const std::string& table_name)
{
    return table_name + c_class_suffix;
}

} // namespace generate
} // namespace catalog
} // namespace gaia
