////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "gaia_internal/db/gaia_ptr.hpp"

#include "db_helpers.hpp"
#include "db_server.hpp"

using namespace gaia::common;
using namespace gaia::common::iterators;

/**
 * Server-side implementation of gaia_ptr here.
 *
 * The server implementation only implements the gaia_ptr methods sufficient for catalog access
 * on the server side.
 *
 * Additional methods are not implemented because they were not needed - attempting to
 * call them will result in linker errors.
 *
 * Feel free to implement additional methods if there is a need to do so.
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

} // namespace db
} // namespace gaia
