////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include "gaia_internal/catalog/catalog.hpp"

#include "gaia_parser.hpp"

namespace gaia
{
namespace catalog
{

void execute(std::vector<std::unique_ptr<ddl::statement_t>>& statements);

void load_catalog(ddl::parser_t& parser, const std::string& ddl_filename);

void load_catalog(const char* ddl_filename);

} // namespace catalog
} // namespace gaia
