/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "rule_checker.hpp"

#include "gaia_catalog.hpp"
#include "gaia_catalog.h"

using namespace gaia::rules;
using namespace gaia::common;
using namespace gaia::catalog;
using namespace std;

//
// Rule exception implementations.
//
invalid_rule_binding::invalid_rule_binding()
{
    m_message = "Invalid rule binding. "
        "Verify that the ruleset_name, rule_name and rule are provided.";
}

duplicate_rule::duplicate_rule(const rule_binding_t& binding, bool duplicate_key_found)
{
    std::stringstream message;
    if (duplicate_key_found)
    {
        message << binding.ruleset_name << "::"
            << binding.rule_name 
            << " already subscribed with the same key "
            "but different rule function.";
    }
    else
    {
        message << binding.ruleset_name << "::"
            << binding.rule_name 
            << " already subscribed to the same rule list.";
    }
    m_message = message.str();
}

initialization_error::initialization_error(bool is_already_initialized)
{
    if (is_already_initialized)
    {
        m_message = "The event manager has already been initialized.";
    }
    else
    {
        m_message = "The event manager has not been initialized yet.";
    }
}

invalid_subscription::invalid_subscription(gaia::db::triggers::event_type_t event_type, const char* reason)
{
    std::stringstream message;
    message << "Cannot subscribe rule to " << (uint32_t)event_type << ". " << reason;
    m_message = message.str();
}

// Table container_id not found.
invalid_subscription::invalid_subscription(gaia_container_id_t container_id)
{
    std::stringstream message;
    message << "Table (container_id:" << container_id << ") "
        << "was not found in the catalog.";
    m_message = message.str();
}

// Field not found.
invalid_subscription::invalid_subscription(gaia_container_id_t container_id, const char* table,
    uint16_t position)
{
    std::stringstream message;
    message << "Field (position:" << position << ") "
        << "was not found in table '" << table << "' "
        << "(container_id:" << container_id << ").";
    m_message = message.str();
}

// Field not active or deprecated.
invalid_subscription::invalid_subscription(gaia_container_id_t container_id, const char* table,
    uint16_t position, const char* field, bool is_deprecated)
{
    std::stringstream message;
    const char * reason = is_deprecated ? "deprecated" : "not marked as active";
    message << "Field '" << field 
        << "' (position:" << position << ")"
        << " in table '" << table 
        << "' (container_id:" << container_id << ")"
        << " is " << reason << ".";
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
void rule_checker_t::check_catalog(gaia_container_id_t container_id, const field_position_list_t& field_list)
{
    auto_transaction_t transaction;
    check_table_container(container_id);
    check_fields(container_id, field_list);
}

// This function assumes that a transaction has been started.
void rule_checker_t::check_table_container(gaia_container_id_t container_id)
{
    bool found_container = false;
    // CONSIDER: when reference code gets generated 
    // then use the list method.
    for (gaia_table_t table = gaia_table_t::get_first() ; 
        table;
        table = table.get_next())
    {
        // The gaia_id() of the gaia_table_t is the container_id.
        if (container_id == table.gaia_id())
        {
            found_container = true;
            break;
        }
    }

    if (!found_container)
    {
        throw invalid_subscription(container_id);
    }
}

// This function assumes that a transaction has been started and that the table
// container_id exists in the catalog.
void rule_checker_t::check_fields(gaia_container_id_t container_id, const field_position_list_t& field_list)
{
    if (0 == field_list.size())
    {
        return;
    }

    // This function assumes that check_table_container was just called
    gaia_table_t gaia_table = gaia_table_t::get(container_id);
    auto field_ids = list_fields(container_id);

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
                // If the field is deprecated or not active then we should
                // not be able to subscribe to it.  If the field is both deprecated
                // and not active, report it as being deprecated.
                if (gaia_field.deprecated() || !gaia_field.active())
                {
                    throw invalid_subscription(
                        container_id,
                        gaia_table.name(),
                        requested_position,
                        gaia_field.name(),
                        gaia_field.deprecated()
                    );
                }
                found_requested_field = true;
                break;
            }
        }

        if (!found_requested_field)
        {
            throw invalid_subscription(
                container_id,
                gaia_table.name(),
                requested_position
            );
        }
    }
}
