// RUN: %clang_cc1 -fsyntax-only -verify -fgaia-extensions %s

ruleset test_table tables(sensor, incubator) // expected-error {{expected '{'}}
{
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

// expected-note@+1 {{to match this '('}}
ruleset test_table_1 : tables(sensor incubator) // expected-error {{expected ')'}}
{
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

ruleset test_table_2 : table123(sensor, incubator) // expected-error {{Invalid Gaia attribute.}}
{
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

ruleset test_table_3 : tables(sensor, incubator) tables(sensor, incubator) // expected-error {{Invalid Gaia attribute.}}
{
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

ruleset test_table_4 : tables() { // expected-error {{Invalid Gaia ruleset attribute.}}
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

ruleset test_table_5 : tables("cbcbc") // expected-error {{expected identifier}}
{
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

ruleset test_table_6 : tables(345) // expected-error {{expected identifier}}
{
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

ruleset test_table_7 : { // expected-error {{Invalid Gaia attribute.}}
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

ruleset test_table_8 : tables(, ) // expected-error {{expected identifier}}
{
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

ruleset test_table_9: tables(sensor, incubator, bogus)     // expected-error {{Table 'bogus' was not found in the catalog.}}
{
    {
        max_temp += min_temp/2;
    }
}

ruleset test_serial_stream_1 : tables(sensor, incubator), serialize() // expected-error {{expected identifier}}
{
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}
// expected-note@+1 {{to match this '('}}
ruleset test_serial_stream_2 : tables(sensor, incubator), serialize(sdfdf, sfdfsf) // expected-error {{expected ')'}}
{
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

ruleset test_serial_stream_3 : tables(sensor, incubator), serialize(, ) // expected-error {{expected identifier}}
{
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

// expected-note@+1 {{to match this '('}}
ruleset test_serial_stream_4 : tables(sensor, incubator), serialize(sdsdf, ) // expected-error {{expected ')'}}
{
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

ruleset test105 tables[actuator] // expected-error {{expected '{'}}
{
    {
        actuator.value=.5;
    }
}

ruleset test109 Fable(actuator) // expected-error {{expected '{'}}
{
    {
        actuator.value=.5;
    }
}

ruleset test110 tables{actuator} // expected-error {{expected '{'}}
{ // expected-error {{expected unqualified-id}}
    {
        actuator.value=.5;
    }
}
