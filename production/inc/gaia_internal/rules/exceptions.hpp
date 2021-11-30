/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/rules/exceptions.hpp"

namespace gaia
{
namespace rules
{

class invalid_rule_binding_internal : public invalid_rule_binding
{
public:
    invalid_rule_binding_internal();
};

class duplicate_rule_internal : public duplicate_rule
{
public:
    duplicate_rule_internal(const char* ruleset_name, const char* rule_name, bool duplicate_key_found);
};

class initialization_error_internal : public initialization_error
{
public:
    initialization_error_internal();
};

class invalid_subscription_internal : public invalid_subscription
{
public:
    // Table type not found.
    explicit invalid_subscription_internal(common::gaia_type_t gaia_type);
    // Invalid event type specified.
    invalid_subscription_internal(db::triggers::event_type_t event_type, const char* reason);
    // Field not found.
    invalid_subscription_internal(common::gaia_type_t gaia_type, const char* table, uint16_t position);
    // Field not active or has been deprecated
    invalid_subscription_internal(
        common::gaia_type_t gaia_type,
        const char* table,
        uint16_t position,
        const char* field_name,
        bool is_deprecated);
};

} // namespace rules
} // namespace gaia
