/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

namespace gaia
{
namespace common
{

/*
 * Change the given string to plural using a rather naive algorithm.
 * This is a workaround necessary to infer the name of the "parent to
 * children" in gaia_relationship. As soon as it will be possible to
 * specify this name in the DDL, this function can be removed.
 *
 * TODO remove after introducing 'create relationship in the DDL.
 */
std::string to_plural(std::string singular_string);

} // namespace common
} // namespace gaia
