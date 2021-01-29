/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "rule_checker.hpp"

#include "gaia/db/catalog.hpp"

#include "gaia_internal/catalog/gaia_catalog.h"

using namespace gaia::rules;
using namespace gaia::common;
using namespace gaia::catalog;
using namespace gaia::direct_access;
using namespace std;

//
// Rule exception implementations.
//
invalid_rule_binding::invalid_rule_binding()
{
    m_message = "Invalid rule binding. Verify that the ruleset_name, rule_name, and rule are provided.";
}

duplicate_rule::duplicate_rule(const rule_binding_t& binding, bool duplicate_key_found)
{
    std::stringstream message;
    if (duplicate_key_found)
    {
        message << binding.ruleset_name << "::" << binding.rule_name
                << " was already subscribed with the same key but a different rule function.";
    }
    else
    {
        message << binding.ruleset_name << "::" << binding.rule_name << " was already subscribed to the same rule list.";
    }
    m_message = message.str();
}

initialization_error::initialization_error()
{
    m_message = "The rules engine has not been initialized yet.";
}

invalid_subscription::invalid_subscription(gaia::db::triggers::event_type_t event_type, const char* reason)
{
    std::stringstream message;
    message << "Cannot subscribe rule to event type '" << static_cast<uint32_t>(event_type) << "'. " << reason;
    m_message = message.str();
}

// Table type not found.
invalid_subscription::invalid_subscription(gaia_type_t gaia_type)
{
    std::stringstream message;
    message << "Table (type: '" << gaia_type << "') was not found in the catalog.";
    m_message = message.str();
}

// Field not found.
invalid_subscription::invalid_subscription(gaia_type_t gaia_type, const char* table, uint16_t position)
{
    std::stringstream message;
    message
        << "Field (position: " << position << ") was not found in table '"
        << table << "' (type: '" << gaia_type << "').";
    m_message = message.str();
}

// Field not active or deprecated.
invalid_subscription::invalid_subscription(gaia_type_t gaia_type, const char* table, uint16_t position, const char* field, bool is_deprecated)
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

//
// Rule Checker implementation.
//
void rule_checker_t::check_catalog(gaia_type_t type, const field_position_list_t& field_list)
{
    auto_transaction_t txn;
    // Find the id of the table defining gaia_type.
    for (const auto& table : catalog::gaia_table_t::list())
    {
        // The gaia_id() of the gaia_table_t is the type id.
        if (type == table.type())
        {
            check_fields(table.gaia_id(), field_list);
            return;
        }
    }

    // Table type not found.
    throw invalid_subscription(type);
}

// This function assumes that a transaction has been started and that the table
// type exists in the catalog.
void rule_checker_t::check_fields(gaia_id_t id, const field_position_list_t& field_list)
{
    if (0 == field_list.size())
    {
        return;
    }

    gaia_table_t gaia_table = gaia_table_t::get(id);
    auto field_ids = list_fields(id);

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
                // If the field is deprecated then we should not be able to subscribe to it.
                // TODO[GAIAPLAT-622] If we ever add a "strict" mode to the database
                // then we should reinstate checking for active fields.
                if (gaia_field.deprecated())
                {
                    throw invalid_subscription(id, gaia_table.name(), requested_position, gaia_field.name(), gaia_field.deprecated());
                }
                found_requested_field = true;
                break;
            }
        }

        if (!found_requested_field)
        {
            throw invalid_subscription(id, gaia_table.name(), requested_position);
        }
    }
}
