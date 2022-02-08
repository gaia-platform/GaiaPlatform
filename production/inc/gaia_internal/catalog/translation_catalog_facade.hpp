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
namespace translation
{
namespace generate
{

class translation_table_facade_t : public gaia::catalog::generate::table_facade_t
{
public:
    static std::string class_name(const std::string& table_name);
};

} // namespace generate
} // namespace translation
} // namespace gaia
