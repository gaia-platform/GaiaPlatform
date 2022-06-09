////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include "gaia/db/events.hpp"
#include "gaia/logger.hpp"
#include "gaia/rules/rules.hpp"

const char c_A = 'A';
const char c_B = 'B';
const char c_None = '\0';

void enter_rule(char group);
void exit_rule(char group);

extern std::atomic<int32_t> g_rule_called;

class rule_monitor_t
{
public:
    rule_monitor_t(
        const char* rule_id,
        char serial_group,
        const char* ruleset_name,
        const char* rule_name,
        gaia::db::triggers::event_type_t event_type,
        gaia::common::gaia_type_t gaia_type)
        : rule_monitor_t(rule_id, serial_group, ruleset_name, rule_name, static_cast<const uint32_t>(event_type), gaia_type)
    {
    }

    rule_monitor_t(
        const char* rule_id,
        char serial_group,
        const char* ruleset_name,
        const char* rule_name,
        uint32_t event_type,
        gaia::common::gaia_type_t gaia_type);

    ~rule_monitor_t();

private:
    void set_group(bool value);
    void enter_rule();
    void exit_rule();

private:
    char m_group;
};
