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
 * This helper class is used by the rules engine at both rule
 * subscription and invocation time.  Currently, the class performs
 * the following checks.
 *
 * Catalog Checks:
 *  Ensure tables and fields that are referenced in a rule are
 *  actually present in the catalog at rule subscription time.
 *
 * Database Checks:
 *  Ensure that an anchor row is valid before invoking a rule.
 *
 * Note that the checks can be disabled by unit tests that do not want
 * to have a dependency on the database.  These tests can intialize
 * the rules engine with custom settings.  See event_manager_settings.hpp
 * and rules_test_helpers.hpp for more information.
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
