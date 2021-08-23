// RUN: %clang_cc1  -fsyntax-only -verify -fgaia-extensions %s

ruleset test11
{
    ruleset test12
    { // expected-error {{Rulesets can not be defined in ruleset scope.}}
        {
            min_temp += @value;
            max_temp += min_temp / 2;
        }
    }
}

ruleset test31
{
    OnDelete(incubator) // expected-error {{Invalid Gaia rule attribute}}
    {

    } // {{expected ';' after top level declarator}}
}

ruleset test32
{
    on_update(tttt) // expected-error {{Field 'tttt' was not found in the catalog.}}
    {
    }
}

ruleset test33
{
    on_update(incubator sensor) // expected-error {{expected ')'}} expected-note {{to match this '('}}
    {
    }
}

ruleset test34
{
    on_update(incubator, sensor.test) // expected-error {{Field 'test' was not found in the catalog.}}
    {
    }
}

ruleset test35
{
    on_update
    { // expected-error {{expected '('}}
    }
}

ruleset test36
{
    on_update()
    { // expected-error {{Invalid Gaia rule attribute.}}
    }
}

ruleset test37
{
    on_update("sensor") // expected-error {{expected identifier}}
    {
    }
}

ruleset test38
{
    on_update(5) // expected-error {{expected identifier}}
    {
    }
}

ruleset test39
{
    on_update(value) // expected-error {{Duplicate field 'value' found in table 'actuator'. Qualify your field with the table name (table.field) to disambiguate field names that occur in more than one table. You can also restrict the list of tables to search by specifying them in the ruleset 'tables' attribute.}}
    {
    }
}

ruleset test40
{
    on_update(incubator), on_update(sensor) // expected-error {{Invalid Gaia rule attribute.}}
    {
    }
}

ruleset test53
{
    on_update(sensor.value.value) // expected-error {{expected ')'}} expected-note {{to match this '('}}
    {
    }
}

ruleset test53
{
    on_update(value) // expected-error {{Duplicate field 'value' found in table 'actuator'. Qualify your field with the table name (table.field) to disambiguate field names that occur in more than one table. You can also restrict the list of tables to search by specifying them in the ruleset 'tables' attribute.}}
    {
    }
}

ruleset test54
{
    on_update(sensor.) // expected-error {{expected identifier}}
    {
    }
}

ruleset test55
{
    on_change(tttt) // expected-error {{Field 'tttt' was not found in the catalog.}}
    {
    }
}

ruleset test56
{
    on_change(incubator sensor) // expected-error {{expected ')'}} expected-note {{to match this '('}}
    {
    }
}

ruleset test57
{
    on_change(incubator, sensor.test) // expected-error {{Field 'test' was not found in the catalog.}}
    {
    }
}

ruleset test58
{
    on_change
    { // expected-error {{expected '('}}
    }
}

ruleset test59
{
    on_change()
    { // expected-error {{Invalid Gaia rule attribute.}}
    }
}

ruleset test60
{
    on_change("sensor") // expected-error {{expected identifier}}
    {
    }
}

ruleset test61
{
    on_change(5) // expected-error {{expected identifier}}
    {
    }
}

ruleset test62
{
    on_change(value) // expected-error {{Duplicate field 'value' found in table 'actuator'. Qualify your field with the table name (table.field) to disambiguate field names that occur in more than one table. You can also restrict the list of tables to search by specifying them in the ruleset 'tables' attribute.}}
    {
    }
}

ruleset test63
{
    on_change(incubator), on_update(sensor) // expected-error {{Invalid Gaia rule attribute.}}
    {
    }
}

ruleset test64
{
    on_change(sensor.value.value) // expected-error {{expected ')'}} expected-note {{to match this '('}}
    {
    }
}

ruleset test65
{
    on_change(value) // expected-error {{Duplicate field 'value' found in table 'actuator'. Qualify your field with the table name (table.field) to disambiguate field names that occur in more than one table. You can also restrict the list of tables to search by specifying them in the ruleset 'tables' attribute.}}
    {
    }
}

ruleset test66
{
    on_change(sensor.) // expected-error {{expected identifier}}
    {
    }
}

ruleset test67
{
    on_insert(tttt) // expected-error {{Field 'tttt' was not found in the catalog.}}
    {
    }
}

ruleset test68
{
    on_insert(incubator sensor) // expected-error {{expected ')'}} expected-note {{to match this '('}}
    {
    }
}

ruleset test69
{
    on_insert(incubator, sensor.test) // expected-error {{Field 'test' was not found in the catalog.}}
    {
    }
}

ruleset test70
{
    on_insert { // expected-error {{expected '('}}
    }
}

ruleset test71
{
    on_insert()
    { // expected-error {{Invalid Gaia rule attribute.}}
    }
}

ruleset test72
{
    on_insert("sensor") // expected-error {{expected identifier}}
    {
    }
}

ruleset test73
{
    on_insert(5) // expected-error {{expected identifier}}
    {
    }
}

ruleset test74
{
    on_insert(value) // expected-error {{Duplicate field 'value' found in table 'actuator'. Qualify your field with the table name (table.field) to disambiguate field names that occur in more than one table. You can also restrict the list of tables to search by specifying them in the ruleset 'tables' attribute.}}
    {
    }
}

ruleset test75
{
    on_insert(incubator), on_update(sensor) // expected-error {{Invalid Gaia rule attribute.}}
    {
    }
}

ruleset test76
{
    on_insert(sensor.value.value) // expected-error {{expected ')'}} expected-note {{to match this '('}}
    {
    }
}

ruleset test77
{
    on_insert(value) // expected-error {{Duplicate field 'value' found in table 'actuator'. Qualify your field with the table name (table.field) to disambiguate field names that occur in more than one table. You can also restrict the list of tables to search by specifying them in the ruleset 'tables' attribute.}}
    {
    }
}

ruleset test78
{
    on_insert(sensor.) // expected-error {{expected identifier}}
    {
    }
}

ruleset test79
{
    on_insert(S:sensor)
    {
        actuator.value += value/2; // expected-error {{Duplicate field 'value' found in table 'actuator'. Qualify your field with the table name (table.field) to disambiguate field names that occur in more than one table. You can also restrict the list of tables to search by specifying them in the ruleset 'tables' attribute.}} \
                                   // expected-error {{use of undeclared identifier 'value'}}
    }
}

ruleset test79 : tables(sensor, actuator)
{
    on_insert(S:sensor)
    {
        actuator.value += sensor.value/2;
    }
}

ruleset test88
{
    on_update(nonsense, no-sense, not-sensible) // expected-error {{expected ')'}} expected-note {{to match this '('}})
    {
    }
}


ruleset test93
{
    on_update(incubator[i]) // expected-error {{expected ')'}} expected-note {{to match this '('}}
    {
        int i = 0;
    }
}

ruleset test94
{
    Onward(incubator) // expected-error {{Invalid Gaia rule attribute}}
    {
        incubator.min_temp = 0.0;
    }
}


ruleset test104
{
    on_insert[A:animal] // expected-error {{expected '('}}
    {
        A.age=5;
    }
}

ruleset test106
{
    on_insert(*incubator) // expected-error {{expected identifier}}
    {
        incubator.min_temp=98.9;
    }
}

ruleset test107
{
    on_insert(do(min_temp)) // expected-error {{expected identifier}}
    {
        age=5;
    }
}

ruleset test108
{
    on_insert(age ? 3 : 4) // expected-error {{expected ')'}} expected-note {{to match this '('}}
    {
        age=5;
    }
}
