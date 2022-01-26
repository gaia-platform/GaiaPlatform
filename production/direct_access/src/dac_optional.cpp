/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/exceptions.hpp"

namespace gaia
{

namespace direct_access
{

gaia::direct_access::optional_val_not_found_internal::optional_val_not_found_internal()
{
    m_message = "Optional has no value set!";
}

} // namespace direct_access
} // namespace gaia
