/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaiac_catalog_facade.hpp"

namespace gaia
{
namespace catalog
{
namespace generate
{

//
// gaiac_incoming_link_facade_t
//

gaiac_incoming_link_facade_t::gaiac_incoming_link_facade_t(const link_facade_t& link)
    : link_facade_t(link){};

std::string gaiac_incoming_link_facade_t::parent_offset() const
{
    return "c_" + from_table() + "_parent_" + field_name();
}

std::string gaiac_incoming_link_facade_t::parent_offset_value() const
{
    return std::to_string(m_relationship.parent_offset());
}

std::string gaiac_incoming_link_facade_t::next_offset() const
{

    return "c_" + from_table() + "_next_" + field_name();
}

std::string gaiac_incoming_link_facade_t::next_offset_value() const
{
    return std::to_string(m_relationship.next_child_offset());
}

//
// gaiac_outgoing_link_facade_t
//

gaiac_outgoing_link_facade_t::gaiac_outgoing_link_facade_t(const link_facade_t& link)
    : link_facade_t(link){};

std::string gaiac_outgoing_link_facade_t::first_offset() const
{
    return "c_" + from_table() + "_first_" + field_name();
}

std::string gaiac_outgoing_link_facade_t::first_offset_value() const
{
    return std::to_string(m_relationship.first_child_offset());
}

std::string gaiac_outgoing_link_facade_t::next_offset() const
{
    return "c_" + to_table() + "_next_" + m_relationship.to_parent_link_name();
}

} // namespace generate
} // namespace catalog
} // namespace gaia
