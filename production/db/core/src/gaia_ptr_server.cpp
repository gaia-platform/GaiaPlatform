/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/db/gaia_ptr.hpp"

#include "db_helpers.hpp"
#include "db_server.hpp"

using namespace gaia::common;
using namespace gaia::common::iterators;

/**
 * Server-side implementation of gaia_ptr here.
 *
 * Unimplemented write-path methods will lead to linker errors when called in code.
 *
 */

namespace gaia
{
namespace db
{

db_object_t* gaia_ptr_t::to_ptr() const
{
    return locator_to_ptr(m_locator);
}

gaia_offset_t gaia_ptr_t::to_offset() const
{
    return locator_to_offset(m_locator);
}

std::shared_ptr<generator_t<gaia_id_t>>
gaia_ptr_t::get_id_generator_for_type(gaia_type_t type)
{
    return server_t::get_id_generator_for_type(type);
}

} // namespace db
} // namespace gaia
