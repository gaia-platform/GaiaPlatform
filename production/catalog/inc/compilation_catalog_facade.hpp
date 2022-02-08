/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_internal/catalog/catalog_facade.hpp"

/*
 * Provides compilation specific facade.
 */

namespace gaia
{
namespace catalog
{
namespace generate
{

class compilation_incoming_link_facade_t : public link_facade_t
{
public:
    explicit compilation_incoming_link_facade_t(const link_facade_t& link);

    std::string parent_offset() const;
    uint16_t parent_offset_value() const;
    std::string next_offset() const;
    uint16_t next_offset_value() const;
    std::string prev_offset() const;
    uint16_t prev_offset_value() const;
};

class compilation_outgoing_link_facade_t : public link_facade_t
{
public:
    explicit compilation_outgoing_link_facade_t(const link_facade_t& link);

    std::string first_offset() const;
    uint16_t first_offset_value() const;
    std::string parent_offset() const;
    uint16_t parent_offset_value() const;
    std::string next_offset() const;
    std::string prev_offset() const;
};

} // namespace generate
} // namespace catalog
} // namespace gaia
