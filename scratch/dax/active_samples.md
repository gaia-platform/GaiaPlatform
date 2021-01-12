
# Active Fields Thoughts
## Goals
The follow samples are based on my opinion of how things should work:

1. Fields need to be marked as active in the catalog AND specified in a rule "prolog".  If a field is not active in the schema than a compile-time error is emitted by the translation engine.  This allows schema designers (or system engineers) the ability to restrict what columns can participate in change events.  I prefer it being in DDL rather than a GRANT clause so that you can quickly see it in DDL.
1. Given that I want to give control of event sources to schema designers then I would also want them to control table level event changes as well in schema.
1. Remove `@` annotations in rule body code itself. 

## Proposal
To address the goals above, I propose the following.
1. Add an `active` keyword to create table.  It must include at least one of the following attributes: `on_insert`, `on_upate`, or `on_delete`.
1. Support annotation of these attributes in the rule prolog.
1. Remove `@` markers in rule entirely.

## Illustrative Examples
Each example shows the DDL and a set of rules that accompany the syntax.

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
Persons.on_Delete
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
```


### Table that [Scenario here]
#### DDL
```
```
#### Rules
```
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
Keep the temperature with the range of the minimum and maximum limits of the incubator.
```
incubator.on_update(max_temp, min_temp),
sensor.on_update(value)
{
    if (!incubator.is_on)
    {
        return;
    }

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
actuator.on_update()
{
    if (actuator.value == c_fan_speed_limit)
    {
        return;
    }

    if (actuator.value > c_fan_threshold * c_fan_speed_limit && sensor.value > incubator.max_temp)
    {
        actuator.value = c_fan_speed_limit;
    }
}
```
