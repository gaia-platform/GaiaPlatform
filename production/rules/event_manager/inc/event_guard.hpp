/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "rules.hpp"

namespace gaia 
{
namespace rules
{

// Guards against reentrant log_event
// calls.  Ensures that we can clean up 
// executing rule state if this class leaves 
// scope either because it exits normally or 
// an exception is thrown.
class event_guard_t
{
public:
    event_guard_t() = delete;
    event_guard_t(
        uint32_t& bitmap,
        const event_type_t& event_type)
        : m_event_bitmap(bitmap)
        , m_event_bit((uint32_t)event_type)
        , m_blocked(false)
    {
        static_assert(sizeof(event_type_t) == sizeof(uint32_t), "bitmap type must be sizeof(uint32_t)");

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
