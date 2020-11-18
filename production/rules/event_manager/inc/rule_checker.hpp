/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "gaia/rules.hpp"

namespace gaia
{
namespace rules
{

/**
 * This helper class interfaces with the catalog to verify rule subscriptions.
 * This class is invoked when the user calls the subscribe_rule API.
 *
 * Post Q2 this functionality may be moved to the catalog manager so that other code can
 * also use it.
  */
class rule_checker_t
{
public:
    void check_catalog(gaia_type_t type, const field_position_list_t& field_list);

private:
    void check_fields(gaia_id_t id, const field_position_list_t& field_list);
};

} // namespace rules
} // namespace gaia
