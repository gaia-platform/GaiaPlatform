/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/catalog/translation_catalog_facade.hpp"

namespace gaia
{
namespace translation
{
namespace generate
{

//
// translation_table_facade_t
//

std::string translation_table_facade_t::class_name(const std::string& table_name)
{
    return table_name + catalog::generate::c_class_suffix;
}

} // namespace generate
} // namespace translation
} // namespace gaia
