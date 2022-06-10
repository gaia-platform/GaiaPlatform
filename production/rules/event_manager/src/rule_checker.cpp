////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "rule_checker.hpp"

#include "gaia/rules/exceptions.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"
#include "gaia_internal/rules/exceptions.hpp"

using namespace gaia::rules;
using namespace gaia::common;
using namespace gaia::catalog;
using namespace gaia::direct_access;
using namespace std;

//
// Rule exception implementations.
//
invalid_rule_binding_internal::invalid_rule_binding_internal()
{
    m_message = "Invalid rule binding. Verify that the ruleset_name, rule_name, and rule are provided.";
}

duplicate_rule_internal::duplicate_rule_internal(const char* ruleset_name, const char* rule_name, bool duplicate_key_found)
{
    std::stringstream message;
    if (duplicate_key_found)
    {
        message << ruleset_name << "::" << rule_name
                << " was already subscribed with the same key but a different rule function.";
    }
    else
    {
        message << ruleset_name << "::" << rule_name << " was already subscribed to the same rule list.";
    }
    m_message = message.str();
}

initialization_error_internal::initialization_error_internal()
{
    m_message = "The rules engine has not been initialized yet.";
}

invalid_subscription_internal::invalid_subscription_internal(gaia::db::triggers::event_type_t event_type, const char* reason)
{
    std::stringstream message;
    message << "Cannot subscribe rule to event type '" << static_cast<uint32_t>(event_type) << "'. " << reason;
    m_message = message.str();
}

// Table type not found.
invalid_subscription_internal::invalid_subscription_internal(gaia_type_t gaia_type)
{
    std::stringstream message;
    message << "Table (type: '" << gaia_type << "') was not found in the catalog.";
    m_message = message.str();
}

// Field not found.
invalid_subscription_internal::invalid_subscription_internal(gaia_type_t gaia_type, const char* table, uint16_t position)
{
    std::stringstream message;
    message
        << "Field (position: " << position << ") was not found in table '"
        << table << "' (type: '" << gaia_type << "').";
    m_message = message.str();
}

// Field not active or deprecated.
invalid_subscription_internal::invalid_subscription_internal(gaia_type_t gaia_type, const char* table, uint16_t position, const char* field, bool is_deprecated)
{
    std::stringstream message;
    const char* reason = is_deprecated ? "deprecated" : "not marked as active";
    message
        << "Field '" << field << "' (position: " << position << ") in table '" << table
        << "' (type: '" << gaia_type << "') is " << reason << ".";
    m_message = message.str();
}

ruleset_not_found::ruleset_not_found(const char* ruleset_name)
{
    std::stringstream message;
    message << "Ruleset '" << ruleset_name << "' not found.";
    m_message = message.str();
}

ruleset_schema_mismatch::ruleset_schema_mismatch(const char* db_name)
{
    std::stringstream message;
    message << "Ruleset is not consistent with schema for DB '" << db_name << "'. Please rebuild the ruleset to match the catalog definition.";
    m_message = message.str();
}

dac_schema_mismatch::dac_schema_mismatch(const char* class_name)
{
    std::stringstream message;
    message << "Class '" << class_name << "' is not consistent with schema in the DB. Please rebuild the Direct Access Classes to match the catalog definition.";
    m_message = message.str();
}

//
// Rule Checker implementation.
//
void rule_checker_t::check_catalog(gaia_type_t type, const field_position_list_t& field_list)
{
    if (!m_enable_catalog_checks)
    {
        return;
    }

    auto_transaction_t txn;
    for (const auto& table : catalog::gaia_table_t::list())
    {
        if (type == table.type())
        {
            check_fields(table, field_list);
            return;
        }
    }

    // Table type not found.
    throw invalid_subscription_internal(type);
}

// This function assumes that a transaction has been started and that the table
// type exists in the catalog.
void rule_checker_t::check_fields(const catalog::gaia_table_t& gaia_table, const field_position_list_t& field_list)
{
    if (0 == field_list.size())
    {
        return;
    }

    auto field_ids = list_fields(gaia_table.gaia_id());

    // Walk through all the requested fields and check them against
    // the catalog fields.  Make sure the field exists, is not deprecated
    // and marked as active.
    for (auto requested_position : field_list)
    {
        bool found_requested_field = false;
        for (gaia_id_t field_id : field_ids)
        {
            gaia_field_t gaia_field = gaia_field_t::get(field_id);
            if (gaia_field.position() == requested_position)
            {
                // If the field is deprecated, then we should not be able to subscribe to it.
                // We also have an "active" attribute for a field that would track whether changes
                // to that column should be allowed to fire a rule.  If we want to enable a "strict"
                // mode then we could add that check here.
                if (gaia_field.deprecated())
                {
                    throw invalid_subscription_internal(
                        gaia_table.type(), gaia_table.name(), requested_position, gaia_field.name(),
                        gaia_field.deprecated());
                }
                found_requested_field = true;
                break;
            }
        }

        if (!found_requested_field)
        {
            throw invalid_subscription_internal(gaia_table.type(), gaia_table.name(), requested_position);
        }
    }
}

// A transaction must be active before calling this function.
bool rule_checker_t::is_valid_row(gaia::common::gaia_id_t row_id)
{
    if (!m_enable_db_checks)
    {
        return true;
    }

    gaia::db::gaia_ptr_t row_ptr = gaia::db::gaia_ptr_t::from_gaia_id(row_id);
    return static_cast<bool>(row_ptr);
}
