
# Active Field Proposal
## Scope
1. DDL syntax for active fields and tables.
1. Rule syntax for describing when a rule should be invoked both in a rule prologue and the rule body.

## Not in scope in this proposal
1. Preconditions (use of `where` in rule annotations to determine whether a rule is fired at all)
1. Ability to bind to pre-commit events.

Even though the above two items are not in scope, we should ensure that nothing in this proposal precludes addition of these constructs.

## DDL Changes

### Current Behavior
Currently a field must be marked as `active` in the DDL to enable rules to fire on changes to that field.  Rules can be bound to row-level events (inserts, deletes, updates) without needing any schema annotations.  Feedback from the hackathon was that having to change the DDL everytime you wanted to reference an active field in the rule added too much friction to the app development cycle.

### Proposed Changes
1. Maintain the `active` keyword on a field to denote that changes to this field will trigger a rule.
1. Add an `active` keyword on a table with a list of *row* level operations that will trigger a rule.
1. By default, annotating schema with either a field or table level `active` keyword is not required.
1. Add a `strict` mode whereby the translation and rules engine will not allow subscribing rules to events that are not marked as `active` in the schema.  In strict mode, a table and field must explicitly be *declared* as active in the schema.  Strict mode is specified at the database level in schema so that tools that use the catalog can operate correctly.

### DDL examples

Create a database with strict mode on.
```
create database my_database strict
```
Add a table that allows rules to be written against *row* inserts, updates, and deletes.
```
create table Persons active(on_insert, on_update, on_delete)
(
      FirstName string,
      LastName string,
      BirthDate int64,
      FaceSignature string,
      IsTrusted bool
);
```
Add a table that allows rules to be written for changes to the `IsTrusted` and `FaceSignature` fields but does not allow writing rules to *row* level operations
```
create table Persons
(
      FirstName string,
      LastName string,
      BirthDate int64,
      FaceSignature string active,
      IsTrusted bool
);
```
Add a table that allows rules to be written for *row* operations as well as to field changes.
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

## Rule Changes
### Current Behavior
An active field referenced in the rule as a r-value will cause that rule to be invoked if the referenced field's value changes.  If the reference to the field is annotated with an `@` symbol, then the value of the field can be read but the rule will not be invoked by changes to that rule.

If a user wants to write a rule that is invoked for *row* level changes (insert, update, delete) then they reference a "mock" field off the table called `LastOperation`.  They can use this field then to specify what event will cause the rule to be invoked.  For example:  if Persons.LastOperation == INSERT.

Fields are required to be marked as `active` in schema.  If a rule does not reference any active fields or does not reference the LastOperation keyword, then the translation engine will fail to compile the rule.
### Proposed Changes
1. Use the `@` decoration of a field to affirm that it is an active field.  This is the opposite of its current behavior.
1. Introduce `OnInsert`, `OnUpdate`, and `OnDelete` annotations to a rule in the prologue of rule, not the rule body, that replace `LastOperation`.  These annotations must be qualified by an existing table name.  The `OnUpdate` keyword may include an optional field list.  If a field list is specified, then the rule is only fired when an update occurs and the specific fields in the field list are changed.  In other words, specifying `OnUpdate` with a field list is a more verbose mechanism of specifying active fields.
1. Rule annotations can be combined to bind the same rule to multiple events.  As today, the translation engine may generate multiple functions if the annotations specify multiple tables or fields from multiple tables.
1. Rule annotations in the prologue can also be combined with `@` designations in the rule body.  However, annotations in the rule prologue must be consistent with `@` usage in the rule body.  If there is a conflict then the translation engine will emit a compile error.  See examples.
1. if `strict` mode is specified, a compile time error will occur if the annotations do not match the schema.  I.e., one could not specify `OnInsert` if the table itself didn't declare `OnInsert` in its schema definition.  One could also not reference an active field if it were not marked as active in the database.


### Valid Rule Examples
The following examples highlight the annotations to specify when a rule would be invoked.

Single-table insert
```
OnInsert(Persons)
{
}
```
Multi-table insert

When a new person is added or a new face scan entry is added,
fire a rule.  Note that there was a suggestion to invert this syntax and allow OnInsert to take a list of tables.  I think this will add confusion for OnUpdate since it can take a list of fields.  
```
OnInsert(Persons, FaceScanLog)
{
}
```
Field change (no rule prologue).
```
{
    if (@IsTrusted)
    {
    }
}
```
The above is equivalent to using the `OnUpdate` keyword with a field list.  Unlike the above rule, however, `IsTrusted` did not have to be referenced as an r-value in the rule body itself to trigger the rule.
```
OnUpdate(IsTrusted)
{
    if (BirthDate > 1/1/2000)
    {
        // Do something.  Note that there is no conflict here.  BirthDate is not marked with the @ annotation.
    }
}
```
Update row change.  This specifies that I want this rule to fire on a change to *any* field in the persons table.  Note that in this case the `OnUpdate` annotation without a field list supercedes the mention of the `IsTrusted` active field.  This is allowed and is not a conflict since implicity *all* fields are allowed.
```
OnUpdate(Persons)
{
    if (@IsTrusted)
    {
    }
}
```
Specify that I want the field to fire for either an insert or a change to an active field.
```
OnInsert(Persons)
{
    if (@IsTrusted)
    {
        // Fired if a new Person is added OR an update occurs and IsTrusted is changed.
    }
}
```
Likewise, the fully explicit syntax would also work.  Note that technically the @ sign on IsTrusted is not needed in the rule body.  However, it is okay to leave it there as it is consistent with the prologue declaration.
```
on_change(IsTrusted)
{
    if (@IsTrusted)
    {
    }
}
```

### Invalid Rule Examples
Compile error since there is no `on_*` keyword or active field reference in the rule.
```
{
    // Must designate at least one field as active using `@` or
    // the translation engine will emit an error since we have no idea
    // what event to bind this rule to.
}
```
Assume `strict` mode is on and there are no active table or active field attributes in the schema.  The following rule definitions will fail:
```
// compile failure: active(on_insert) not specified on Persons table.
Persons.OnInsert
{

}

// compile failure:  IsTrusted is not marked as an active field in the Persons table.
{
    if (@IsTrusted){...}
}
```
For the following examples, assume that `strict` mode is off (or the schema correctly designates all the tables and fields as active).

Attempting to reference a value from a deleted row.
```
Persons.OnDelete
{
    // compile failure:  can't read a value from a deleted row.
    if(BirthDate > 1/1/2000) {...}
}
```
Active field list inconsistencies between rule prologue and `@` annotations
```
Persons.OnUpdate(IsTrusted)
{
    // compile failure:  BirthDate is not declared as an active field in the rule prologue
    if (@BirthDate > 1/1/2000){...}

    // Note that this active field reference is fine because we have no conflicting prologue 
    // directive on the FaceScanLog table.
    if (@FaceScanLog.ScanSignature == FaceSignature) {...}
}
```
# Alternate Proposal
1. Everything specified in rule prologue
1. Bind to table row level events (insert, update, delete), bind to field level events. special case of fields+insert?

## Examples
```
// Active fields
[IsTrusted]
{
}

// Multiple active fields.
[IsTrusted, FaceSignature]
{
}

// Qualified Active Fields
[Persons(IsTrusted, FaceSignature)]
{
}

// Or
[Incubator.is_on, Sensor.value, ...]
{
}

// Row event
// Update to Persons
[*Person]
{
}



// Update to Persons or FaceScanLog
[+-*Person, FaceScanLog]
{
}

// Fire on Insert
[+Person]
modifiers:  +, -,*
{
}

// Fire on Insert and Update
[*+Person]

// Fire on Delete
[-Person]
{
}

// Active field for insert
[+Person(IsTrusted)]
{
}

// Or
[+Person, IsTrusted]


[+-Person*] versus [+-Person(*)]

LBracket <rule_entity> [, <rule_entity> ...] RBracket
<rule_entity> := [+][-] TABLE_NAME[<field_list>] | <field_entity>
<field_entity> := [TABLE_NAME.]FIELD_NAME
<field_list> := * | (FIELD_NAME [, FIELD_NAME ...])
// Write examples in wiki?  or googledocs ...



// Examples
// Fire rule when update happens to Persons or FaceScanLog
[+-Persons*, FaceScanLog(*)]

//update
[Persons*]

// active field
[Persons(IsTrusted)]

// on change to IsTrusted
[+Persons(IsTrusted)]

versus
[+Persons.IsTrusted]
// Unqualified?
[+IsTrusted]
// is shorthand for below
[+Persons, IsTrusted]
[+Persons(IsTrusted)]
[+Persons.IsTrusted]




LBracket Entity [, Entity...] RBracket

Entity := 

[+Incubator(max_value, min_value), Sensor(value)]
// if no ambiguity, then equivalent to:
[+max_value, min_value, value]

+Person(IsTrusted)



// Table can have insert, update, or delete events.  These are row-level events.
[Persons]

[Persons.IsTrusted]
{

}

[Persons(IsTrusted+, FaceSignature)]

[IsTrusted+, FaceScanLog]
{
}
```





# Feedback for David
## ActiveFields and On
1. Make rooom for a strict mode
1. Rules for Interplay of OnX and active fields (?)
1. Are OnX combinable?  David answer:  NO, no on-delete.  They do stack on top of each other.
1. Concern about arguments for OnX being either tables names or field names.  Rules should be:
    * OnInsert, OnDelete are only tables
    * OnUpdate, OnChange are tables or fields
    * OnChange(TableA, TableA.FieldA) // What does this mean?  Versus (OnInsert(TableA), OnUpdate(FieldA))
1. Where clause with equality based predicates is a can of worms.  What does it mean for rules that reference multiple tables?  Incubator/sensor example

## Table.Where
1. Table.Where is also a With? So...
```
// Only iterate over employees that match the criteria
Employees.Where(HireDate >= 1/20/20) {
    Salary += $1.
}
```
1. How about an empty navigation or iterate all over.  Empty where or just table name?
```
// Iterate over all employees in the b
Employees {
  Salary += $1
}
```
onchange(sensor.value == a && incubator.max_value < 5)

# Explicit Navigation:
In Where
Without a Where
Examples should use hackathon schema.
```
OnChange()
```

Registration->Event.Name
Registration->Student->Person.Name

Family->Father->Person.Name
Family->Mother->Person.Name

Building.Rooms.Where(...)
{

}

Rooms.Building
{

}

// 
Building.Where(Name == "Lincoln" && Rooms.Capacity < 10)
{
    // I am in the Lincoln Building at the first room with a capacity < 10.
    // But what am I sitting on here?  Do I have "two" anchor rows in the field?
}



Check out this syntax:
```
int main(){
  std::vector<int> ints{0, 1, 2, 3, 4, 5};
  auto even = [](int i){ return 0 == i % 2; };
  auto square = [](int i) { return i * i; };
 
  for (int i : ints | std::view::filter(even) | 
                      std::view::transform(square)) {
    std::cout << i << ' ';             // 0 4 16
  }
}
```