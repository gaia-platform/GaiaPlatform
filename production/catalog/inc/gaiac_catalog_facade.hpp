/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_internal/catalog/catalog_facade.hpp"

/*
 * Provides gaiac specific facade.
 */

namespace gaia
{
namespace catalog
{
namespace generate
{

class gaiac_incoming_link_facade_t : public link_facade_t
{
public:
    gaiac_incoming_link_facade_t(const gaia_relationship_t& relationship, bool is_from_parent);
    gaiac_incoming_link_facade_t(const link_facade_t& link);

    std::string parent_offset() const;
    std::string parent_offset_value() const;
    std::string next_offset() const;
    std::string next_offset_value() const;
};

class gaiac_outgoing_link_facade_t : public link_facade_t
{
public:
    gaiac_outgoing_link_facade_t(const gaia_relationship_t& relationship, bool is_from_parent);
    gaiac_outgoing_link_facade_t(const link_facade_t& link);

    std::string first_offset() const;
    std::string first_offset_value() const;
    std::string next_offset() const;
};

} // namespace generate
} // namespace catalog
} // namespace gaia
