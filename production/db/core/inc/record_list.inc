/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

record_range_t* record_range_t::next_range() const
{
    return m_next_range;
}

bool record_range_t::is_full() const
{
    return m_next_available_index >= m_record_list->get_range_size();
}

bool record_iterator_t::at_end() const
{
    return current_range == nullptr;
}

size_t record_list_t::get_range_size() const
{
    return m_range_size;
}
