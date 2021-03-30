/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "base_memory_manager.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

base_memory_manager_t::base_memory_manager_t()
{
    m_base_memory_address = nullptr;
    m_start_memory_offset = c_invalid_address_offset;
    m_total_memory_size = 0;
}

} // namespace memory_manager
} // namespace db
} // namespace gaia
