# Ping Pong Test Workload

## Goals

The main goals of this workload are as follows:
- provide a test of "tight" loops

## Description

This workload provides for two scenarios: the ping-pong scenario and the marco polo scenario.

The ping-pong scenario is very simple.
The timestamp value is set on the single row in the `ping` table.
A rule exists to respond to the setting of that row by setting the single row in the `pong` table.
The monitoring of the scenario pauses until it can confirm that the `pong` table has been modified.

The marco polo scenario is a small variation on the ping-pong scenario.
Instead of an external pause, this scenario keeps an internal count and a goal to race towards.
The scenario is considered finished once the second table reports that it has achieved that goal.

## Base Level

The base level of these integration tests is a very simple request-acknowledge pattern with a minimum of overhead.  The DDL is very basic:

```ddl
create table if not exists pong_table (
    name string,
    rule_timestamp uint64
);

create table if not exists ping_table (
    name string,
    step_timestamp uint64
);
```

as is the ruleset:

```c++
    on_update(ping_table.step_timestamp)
    {
        /pong_table.rule_timestamp = ping_table.step_timestamp;
        g_rule_1_tracker++;
    }
```

In the step for this workload, the global monotonic timestamp is incremented by one and the singular entry in the `ping_table` is updated to that timestamp.
