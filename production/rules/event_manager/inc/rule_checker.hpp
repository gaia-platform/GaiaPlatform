/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "gaia/rules/rules.hpp"

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
    // By default, enable all checks.
    rule_checker_t()
        : rule_checker_t(true, true)
    {
    }

    rule_checker_t(bool enable_catalog_checks, bool enable_db_checks)
        : m_enable_catalog_checks(enable_catalog_checks)
        , m_enable_db_checks(enable_db_checks)
    {
    }

    void check_catalog(common::gaia_type_t type, const common::field_position_list_t& field_list);
    bool is_valid_row(common::gaia_id_t row_id);

private:
    void check_fields(common::gaia_id_t id, const common::field_position_list_t& field_list);

private:
    bool m_enable_catalog_checks;
    bool m_enable_db_checks;
};

} // namespace rules
} // namespace gaia
