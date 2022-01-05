/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_internal/catalog/catalog_facade.hpp"

/*
 * Provides gaiat specific facade.
 */

namespace gaia
{
namespace catalog
{
namespace generate
{

class gaiat_table_facade_t : public table_facade_t
{
public:
    static std::string class_name(const std::string& table_name);
};

} // namespace generate
} // namespace catalog
} // namespace gaia
