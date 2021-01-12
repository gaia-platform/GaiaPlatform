
# Active Field Thoughts
## Goals/Proposal
The follow samples are based on my opinion of how things should work:

1. Fields need to be marked as active in the catalog AND specified in a rule "prolog".  If a field is not active in the schema than a compile-time error is emitted by the translation engine.  This allows schema designers (or system engineers) the ability to restrict what columns can participate in change events.  I prefer it being in DDL rather than a GRANT clause so that you can quickly see it in DDL.
1. Given that I want to give control of event sources to schema designers then I would also want them to control table level event changes as well in schema.
1. Enable pre-conditions for rules (this is filtering outside of the rule-body itself).
1. Remove the need for `@` annotations in rule body code itself. 

## Fleshing out the Proposal

### DDL Syntax
To address the goals above, I propose adding an `active` keyword to create table.  It must include at least one of the following attributes: `on_insert`, `on_update`, or `on_delete`. The `active` keyword on a column still remains.  It is possible to mark a column as active without requiring `active(on_update)` on the enclosing table.

### Rule Defintions
1. Require a prolog for each rule that specifies why the rule will be fired.  The prolog allows the following annotations:
    * Active *Table* annotations:  table_name.on_insert, table_name.on_update, table_name.on_delete.
    * Active *Field* are listed as part of the `on_update` attribute. That is. table_name.on_update(active_column1, active_column2, ...)
    * Event reasons can be combined.  I.e. table1.on_insert, table2.on_update. Semantically, a rule marked with more than one event source will fire if ANY of the events are caused.
1. Filter criteria can be added using a `where` clause construct. I believe that it is more straightforward to separate this construct from the active field list of the `on_...` construct.  A where clause could conceivably be attached to the table and unattached.  More on this distinction in the next session.  Having an explicit `where` keyword also gives us room to expand to other keywords if needed.

### Filtering
Filtering brings up complexities that should be addressed.  Keep in mind that the filter in this context is a pre-condition for calling the rule.  This document does not address filtering within the body of a rule except that the filter syntax should be largely the same.

The same rule can be fired due to multiple events.  For example, one could bind a single rule to two different table type updates.  The rule below fires if either the incubator temperature range changes or the sensor value changes.

```
incubator.on_update(max_value, min_value)
sensor.on_update(value)
{
    ...
}
```
Let's add a filter criteria now so that the rule only runs if the incubator is on.  Note that since we have specified an active field list, we don't need to require that `is_on` be active of not.
```
incubator.on_update(max_value, min_value).where(is_on == true)
sensor.on_update(value)
{
    ...
}
```
This begs the question:  when exactly will thie rule be called?  Does the where clause impact just the incubator update event or also the sensor update event?  The intent is that this rule never fires if the incubator is off.  It is not obvious to me that the above suggests that?  Therefore, I believe we need a "rule-wide" `where` construct.  This would look something like:
```
incubator.on_update(max_value, min_value)
sensor.on_update(value)
where (incubator.is_on == true)
{
    ...
}
```
Do we need to support pre-conditions that are only scoped to a single event in a multi-event rule?  I'd prefer not to support *both* `table.on_update(...).where` and a rule-wide `where` at the same time.  If we did support both then we'd need to understand the order in which multiple filters are applied and which takes precedence in case of a conflict.  Although, not ideal, if the user really wanted a where clause only applied to a single event then they could write a separate rule for this pre-condition.

Another note about that pre-conditions is that they will only be evaluated against post-commit values and not pre-commit values.  This means that it doesn't make sense to have conditionals attached to an `on_delete` attribute.  Furthermore, since the filter criteria is evaluated post commit time, transactions with multiple changes to the same row, will only evaluate against values from the last operation to that row.

## Illustrative Examples
Each example shows the DDL and a set of rules that accompany the syntax above.

### Table that publishes no events.
#### DDL
```
create table Persons
(
      Name string,
      LastName string,
      BirthDate int64,
      FaceSignature string,
      IsTrusted bool
);
```
#### Rules
```
// Any attempt to write a rule against Persons would cause a compile error.  The following
// are all invalid.
Persons.on_insert
{
}

Persons.on_delete
{
}

Persons.on_update
{
}

Persons.on_update(IsTrusted)
{
}
```

### Table that publishes an insert event
#### DDL
```
create table Persons active(on_insert)
(
      FirstName string,
      LastName string,
      BirthDate int64,
      FaceSignature string,
      IsTrusted
);
```
#### Rules
```
Persons.on_insert
{
    // Do something when a Person is inserted.
}

Persons.on_update
{
    // Compile failure.  Cannot subscribe to update events from Persons table.
}

Persons.on_update(IsTrusted)
{
    // Compile failure.  IsTrusted is not marked as active.
}
```
### Table that publishes insert, update, and delete events
#### DDL
```
create table Persons active(on_insert, on_update, on_delete)
(
      FirstName string,
      LastName string,
      BirthDate int64,
      FaceSignature string,
      IsTrusted
);
```
#### Rules
```
Persons.on_insert,
Persons.on_update,
Persons.on_delete
{
    // Subscribe to insert, update, or delete operations
    ...
}

// Or individually
Persons.on_delete
{
    // Only fire when a Persons row is deleted.
}

Persons.on_update
{
    // Subscribe to any column update.
}

Persons.on_update(FaceSignature)
{
    // Compile failure.  FaceSignature is not marked as active.
}
```
### Table that publishes active fields
#### DDL
```
create table Persons
(
      FirstName string,
      LastName string,
      BirthDate int64,
      FaceSignature string active,
      IsTrusted bool active
);
```
#### Rules
```
Persons.on_update(FaceSignature, IsTrusted)
{
    // If either FaceSignature or IsTrusted change, then invoke rule.
}

Persons.on_update(IsTrusted)
{
    // Only fire if IsTrusted Changes
}

Persons.on_update(IsTrusted)
{
    if (Persons.FaceSignatue != get_latest_scan())
    {
        // Compile failure.  FaceSignature was not specified in active field list of the rule prolog.
    }
}
```
### Table that publishes both table and column events
#### DDL
```
create table Persons active(on_insert, on_update)
(
      FirstName string,
      LastName string,
      BirthDate int64,
      FaceSignature string active,
      IsTrusted bool active
);
```
#### Rules
```
Persons.on_insert
{
    // Fire rule on any insert
}

Persons.on_update(IsTrusted)
{
    // Fire rule if IsTrusted changes due to an update.
}

Persons.on_insert,
Persons.on_update(IsTrusted)
{
    // Fire rule if IsTrusted changes or a Person is inserted.
}

Persons.on_update
{
    // Fire rule if any column is updated
}

Persons.on_update
Persons.on_update(IsTrusted)
{
    // Fires if any column is updated. The second Persons.on_update(IsTrusted) has no effect.
    // Maybe we should warn in this case.
}
```

### Table with filter expressions
Note that filter expressions `where` clause are only specified in rules and not DDL.  DDL is just repeated here from above for reference.
#### DDL
```
create table Persons active(on_insert, on_update, on_delete)
(
      FirstName string,
      LastName string,
      BirthDate int64,
      FaceSignature string active,
      IsTrusted bool active
);
```
#### Rules
```
Persons.on_insert
where (Persons.BirthDate < 1/1/2000)
{
    // A person older than 21 has just been added.
}

Persons.on_update
where(Persons.BirthDate < 1/1/2000)
{
    // Called when any column is changed and the filter criteria is met.
}

Persons.on_update(IsTrusted)
where(Persons.BirthDate > 12/31/1999)
{
    // Called when the IsTrusted column is changed and the filter criteria is met.
}

Persons.on_update(IsTrusted, FaceSignature)
where(Persons.BirthData > 12/31/1999)
{
    // Called when either the IsTrusted or FaceSignature column is changed and the filter criteria is met.
}

Persons.on_update(FirstName).where(...)
{
    // Compile error.  FirstName is not an active field.
}
```

## Appendix - Further Examples

Whenever a room is added, then adjust its Restricted capacity to the PercentFull restriction.
```
Rooms.on_insert
{
    Rooms.RestrictedCapacity = Rooms.Capacity * (Restrictions.PercentFull / 100.0);
}
```
If the PercentFull value changes, then updated the restricted capacity of all rooms at the
university.  Note that RestrictedCapacity is off Rooms.
```
Restrictions.on_update(PercentFull)
{
    Rooms.RestrictedCapacity = Rooms.Capacity * (Restrictions.PercentFull / 100.0);
}
```
Since the above rule is doing the same thing for inserts and updates, we'd like to combine them.  Note that combining two `on` clauses is an OR not an AND.
```
Restrictions.on_update(PercentFull),
Rooms.on_insert()
{
    Rooms.RestrictedCapacity = Rooms.Capacity * (Restrictions.PercentFull / 100.0);
}
```
If a restriction is removed, reinstate the full capacity of all rooms.
```
Restrictions.on_delete()
{
    Rooms.RestrictedCapacity = Rooms.Capacity;
}
```
Keep the temperature with the range of the minimum and maximum limits of the incubator.  Only fire this rule if the incubator is on.  Note that this is subtle because a change to the sensor value would have to query the incubator table to see if the filter condition is met.
```
incubator.on_update(max_temp, min_temp)
sensor.on_update(value)
where(incubator.is_on == true)
{
    if (sensor.value >= incubator.max_temp)
    {
        actuator.value = min(c_fan_speed_limit, @actuator.value + c_fan_speed_increment);
        actuator.timestamp = g_timestamp;
    }
    else if (sensor.value <= incubator.min_temp)
    {
        actuator.value = max(0.0f, @actuator.value - (2*c_fan_speed_increment));
        actuator.timestamp   = g_timestamp;
    }
}
```
If the fan is at 70% of its limit and the temperature is still too hot then set the fan to its maximum speed.
```
actuator.on_update(value)
where(actuator.value != c_fan_speed_limit)
{
    if (actuator.value > c_fan_threshold * c_fan_speed_limit && sensor.value > incubator.max_temp)
    {
        actuator.value = c_fan_speed_limit;
    }
}
```
