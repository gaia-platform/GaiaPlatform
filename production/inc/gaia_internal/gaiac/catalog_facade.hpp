////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include "gaia_internal/catalog/catalog_facade.hpp"

/*
 * Provides gaiac specific facade.
 */

namespace gaia
{
namespace catalog
{
namespace gaiac
{

class incoming_link_facade_t : public generate::link_facade_t
{
public:
    explicit incoming_link_facade_t(const link_facade_t& link);

    std::string parent_offset() const;
    uint16_t parent_offset_value() const;
    std::string next_offset() const;
    uint16_t next_offset_value() const;
    std::string prev_offset() const;
    uint16_t prev_offset_value() const;
};

class outgoing_link_facade_t : public generate::link_facade_t
{
public:
    explicit outgoing_link_facade_t(const link_facade_t& link);

    std::string first_offset() const;
    uint16_t first_offset_value() const;
    std::string parent_offset() const;
    uint16_t parent_offset_value() const;
    std::string next_offset() const;
    std::string prev_offset() const;
};

} // namespace gaiac
} // namespace catalog
} // namespace gaia
