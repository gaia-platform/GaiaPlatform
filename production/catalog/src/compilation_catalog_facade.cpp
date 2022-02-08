/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "compilation_catalog_facade.hpp"

namespace gaia
{
namespace catalog
{
namespace generate
{

//
// compilation_incoming_link_facade_t
//

compilation_incoming_link_facade_t::compilation_incoming_link_facade_t(const link_facade_t& link)
    : link_facade_t(link){};

std::string compilation_incoming_link_facade_t::parent_offset() const
{
    return "c_" + from_table() + "_parent_" + field_name();
}

uint16_t compilation_incoming_link_facade_t::parent_offset_value() const
{
    return m_relationship.parent_offset();
}

std::string compilation_incoming_link_facade_t::next_offset() const
{

    return "c_" + from_table() + "_next_" + field_name();
}

uint16_t compilation_incoming_link_facade_t::next_offset_value() const
{
    return m_relationship.next_child_offset();
}

std::string compilation_incoming_link_facade_t::prev_offset() const
{
    return "c_" + from_table() + "_prev_" + field_name();
}

uint16_t compilation_incoming_link_facade_t::prev_offset_value() const
{
    return m_relationship.prev_child_offset();
}

//
// compilation_outgoing_link_facade_t
//

compilation_outgoing_link_facade_t::compilation_outgoing_link_facade_t(const link_facade_t& link)
    : link_facade_t(link){};

std::string compilation_outgoing_link_facade_t::first_offset() const
{
    return "c_" + from_table() + "_first_" + field_name();
}

uint16_t compilation_outgoing_link_facade_t::first_offset_value() const
{
    return m_relationship.first_child_offset();
}

std::string compilation_outgoing_link_facade_t::parent_offset() const
{
    return "c_" + to_table() + "_parent_" + m_relationship.to_parent_link_name();
}

uint16_t compilation_outgoing_link_facade_t::parent_offset_value() const
{
    return m_relationship.parent_offset();
}

std::string compilation_outgoing_link_facade_t::next_offset() const
{
    return "c_" + to_table() + "_next_" + m_relationship.to_parent_link_name();
}

std::string compilation_outgoing_link_facade_t::prev_offset() const
{
    return "c_" + to_table() + "_prev_" + m_relationship.to_parent_link_name();
}

} // namespace generate
} // namespace catalog
} // namespace gaia
