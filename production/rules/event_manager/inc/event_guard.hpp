/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <unordered_map>
#include "rules.hpp"
#include "events.hpp"

using namespace gaia::db::triggers;

namespace gaia 
{
namespace rules
{

typedef std::unordered_map<common::gaia_type_t, 
    std::unordered_map<const char*, uint32_t>> field_event_guard_t; 
typedef std::unordered_map<common::gaia_type_t, uint32_t> database_event_guard_t;
static_assert(sizeof(event_type_t) == sizeof(uint32_t), "bitmap type must be sizeof(uint32_t)");

// Guards against reentrant log_event.  Ensures that we can clean up 
// executing rule state if this class leaves scope either because it 
// exits normally or an exception is thrown.
class event_guard_t
{
public:
    event_guard_t() = delete;

    // Note that this will insert <gaia_type_t, uint32_t> entry
    // if the gaia_type is not found on lookup.  The same behavior also 
    event_guard_t(database_event_guard_t& database_event_guard, 
        gaia_type_t gaia_type, 
        event_type_t event_type)
    : m_event_bitmap(database_event_guard[gaia_type])
    {
        set_enter_state(event_type);
    }

    // Note that this will insert <gaia_type_t, field_event_bitmap_t> 
    // entry if the gaia_type is not found on lookup.  A 
    // <const char*, uint32_t> entry will also be inserted in the field_event_bitmap.
    event_guard_t(field_event_guard_t& field_event_guard, 
        gaia_type_t gaia_type,
        const char* field,
        event_type_t event_type)
    : m_event_bitmap(field_event_guard[gaia_type][field])
    {
        set_enter_state(event_type);
    }

    void set_enter_state(event_type_t event_type)
    {
        m_blocked = false;
        m_event_bit = (uint32_t)event_type;

        // Check whether the event is already
        // running.  If not, then put the
        // event in the running state.
        if (m_event_bit == (m_event_bitmap & m_event_bit))
        {
            m_blocked = true;
        }
        else
        {
            m_event_bitmap |= m_event_bit;
        }
    }

    ~event_guard_t()
    {
        // Clear the running bit for
        // this event if we set it.
        if (!m_blocked)
        {
           m_event_bitmap &= (~m_event_bit);
        }
    }

    bool is_blocked()
    {
        return m_blocked;
    }

private:
    uint32_t& m_event_bitmap;
    uint32_t m_event_bit;
    bool m_blocked;
};


} // namespace rules
} // namespace gaia
