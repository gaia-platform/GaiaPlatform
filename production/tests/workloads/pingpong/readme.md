# Ping Pong Integration Test

## Base Level

The base level of these integration tests is a very simple request-acknowledge
pattern with a minimum of overhead.  The DDL is very basic:

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

In the step for this workload, the global monotonic timestamp is incremented
by one and the singular entry in the `ping_table` is updated to that timestamp.
