/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_internal/catalog/catalog_facade.hpp"

/*
 * Provides translation specific facade.
 */

namespace gaia
{
namespace catalog
{
namespace translation
{

class translation_table_facade_t : public gaia::catalog::generate::table_facade_t
{
public:
    static std::string class_name(const std::string& table_name);
};

} // namespace translation
} // namespace catalog
} // namespace gaia
