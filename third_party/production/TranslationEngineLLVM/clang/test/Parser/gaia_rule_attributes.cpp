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
    OnUpdate(tttt) // expected-error {{Field 'tttt' was not found in the catalog.}}
    {
    }
}

ruleset test33
{
    OnUpdate(incubator sensor) // expected-error {{expected ')'}} expected-note {{to match this '('}}
    {
    }
}

ruleset test34
{
    OnUpdate(incubator, sensor.test) // expected-error {{Field 'test' was not found in the catalog.}}
    {
    }
}

ruleset test35
{
    OnUpdate
    { // expected-error {{expected '('}}
    }
}

ruleset test36
{
    OnUpdate()
    { // expected-error {{Invalid Gaia rule attribute.}}
    }
}

ruleset test37
{
    OnUpdate("sensor") // expected-error {{expected identifier}}
    {
    }
}

ruleset test38
{
    OnUpdate(5) // expected-error {{expected identifier}}
    {
    }
}

ruleset test39
{
    OnUpdate(value) // expected-error {{Duplicate field 'value'.}}
    {
    }
}

ruleset test40
{
    OnUpdate(incubator), OnUpdate(sensor) // expected-error {{Invalid Gaia rule attribute.}}
    {
    }
}

ruleset test53
{
    OnUpdate(sensor.value.value) // expected-error {{expected ')'}} expected-note {{to match this '('}}
    {
    }
}

ruleset test53
{
    OnUpdate(value) // expected-error {{Duplicate field 'value'}}
    {
    }
}

ruleset test54
{
    OnUpdate(sensor.) // expected-error {{expected identifier}}
    {
    }
}

ruleset test55
{
    OnChange(tttt) // expected-error {{Field 'tttt' was not found in the catalog.}}
    {
    }
}

ruleset test56
{
    OnChange(incubator sensor) // expected-error {{expected ')'}} expected-note {{to match this '('}}
    {
    }
}

ruleset test57
{
    OnChange(incubator, sensor.test) // expected-error {{Field 'test' was not found in the catalog.}}
    {
    }
}

ruleset test58
{
    OnChange
    { // expected-error {{expected '('}}
    }
}

ruleset test59
{
    OnChange()
    { // expected-error {{Invalid Gaia rule attribute.}}
    }
}

ruleset test60
{
    OnChange("sensor") // expected-error {{expected identifier}}
    {
    }
}

ruleset test61
{
    OnChange(5) // expected-error {{expected identifier}}
    {
    }
}

ruleset test62
{
    OnChange(value) // expected-error {{Duplicate field 'value'.}}
    {
    }
}

ruleset test63
{
    OnChange(incubator), OnUpdate(sensor) // expected-error {{Invalid Gaia rule attribute.}}
    {
    }
}

ruleset test64
{
    OnChange(sensor.value.value) // expected-error {{expected ')'}} expected-note {{to match this '('}}
    {
    }
}

ruleset test65
{
    OnChange(value) // expected-error {{Duplicate field 'value'}}
    {
    }
}

ruleset test66
{
    OnChange(sensor.) // expected-error {{expected identifier}}
    {
    }
}

ruleset test67
{
    OnInsert(tttt) // expected-error {{Field 'tttt' was not found in the catalog.}}
    {
    }
}

ruleset test68
{
    OnInsert(incubator sensor) // expected-error {{expected ')'}} expected-note {{to match this '('}}
    {
    }
}

ruleset test69
{
    OnInsert(incubator, sensor.test) // expected-error {{Field 'test' was not found in the catalog.}}
    {
    }
}

ruleset test70
{
    OnInsert { // expected-error {{expected '('}}
    }
}

ruleset test71
{
    OnInsert()
    { // expected-error {{Invalid Gaia rule attribute.}}
    }
}

ruleset test72
{
    OnInsert("sensor") // expected-error {{expected identifier}}
    {
    }
}

ruleset test73
{
    OnInsert(5) // expected-error {{expected identifier}}
    {
    }
}

ruleset test74
{
    OnInsert(value) // expected-error {{Duplicate field 'value'.}}
    {
    }
}

ruleset test75
{
    OnInsert(incubator), OnUpdate(sensor) // expected-error {{Invalid Gaia rule attribute.}}
    {
    }
}

ruleset test76
{
    OnInsert(sensor.value.value) // expected-error {{expected ')'}} expected-note {{to match this '('}}
    {
    }
}

ruleset test77
{
    OnInsert(value) // expected-error {{Duplicate field 'value'}}
    {
    }
}

ruleset test78
{
    OnInsert(sensor.) // expected-error {{expected identifier}}
    {
    }
}

ruleset test79
{
    OnInsert(S:sensor)
    {
        actuator.value += value/2; // expected-error {{use of undeclared identifier 'value'}} \
                                   // expected-error {{Ambiguous reference to field 'value'}} \
                                   // expected-note {{Field: 'sensor.value'.}} \
                                   // expected-note {{Field: 'actuator.value'}}
    }
}

ruleset test79 : Table(sensor, actuator)
{
    OnInsert(S:sensor)
    {
        actuator.value += sensor.value/2;
    }
}

ruleset test88
{
    OnUpdate(nonsense, no-sense, not-sensible) // expected-error {{expected ')'}} expected-note {{to match this '('}})
    {
    }
}


ruleset test93
{
    OnUpdate(incubator[i]) // expected-error {{expected ')'}} expected-note {{to match this '('}}
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
    OnInsert[A:animal] // expected-error {{expected '('}}
    {
        A.age=5;
    }
}

ruleset test106
{
    OnInsert(*incubator) // expected-error {{expected identifier}}
    {
        incubator.min_temp=98.9;
    }
}

ruleset test107
{
    OnInsert(do(min_temp)) // expected-error {{expected identifier}}
    {
        age=5;
    }
}

ruleset test108
{
    OnInsert(age ? 3 : 4) // expected-error {{expected ')'}} expected-note {{to match this '('}}
    {
        age=5;
    }
}
