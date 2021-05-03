/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

record_range_t* record_range_t::next_range()
{
    return m_next_range;
}

bool record_range_t::has_deletions()
{
    return m_has_deletions;
}

bool record_range_t::is_full() const
{
    return m_next_available_index >= m_range_size;
}

bool record_iterator_t::at_end()
{
    return current_range == nullptr;
}
