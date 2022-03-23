----------------------------------------------------
-- Copyright (c) Gaia Platform LLC
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
----------------------------------------------------

database behave

table node (
    -- a human readable name
    name string,
    -- success, failure, running, idle
    status uint8,
    -- selector, sequence, action
    node_type uint8,

    tick_flag bool,

    argument uint8,

    order uint8,

    children references node[],
    parent references node
);

table check_temperature_trigger (
)
table set_fan_speed_trigger (
)
table check_fan_speed_trigger (
)
table adjust_fan_speed_trigger (
)

create relationship if not exists check_temperature_trigger (
    node.check_temperature -> check_temperature_trigger,
    check_temperature_trigger.parent_node -> node
);

create relationship if not exists set_fan_speed_trigger (
    node.set_fan_speed -> set_fan_speed_trigger,
    set_fan_speed_trigger.parent_node -> node
);

create relationship if not exists check_fan_speed_trigger (
    node.check_fan_speed -> check_fan_speed_trigger,
    check_fan_speed_trigger.parent_node -> node
);

create relationship if not exists adjust_fan_speed_trigger (
    node.adjust_fan_speed -> adjust_fan_speed_trigger,
    adjust_fan_speed_trigger.parent_node -> node
);
