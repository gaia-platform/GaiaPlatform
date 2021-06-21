// RUN: %clang_cc1 -fsyntax-only -verify -fgaia-extensions %s

ruleset test_table Table(sensor, incubator) // expected-error {{expected '{'}}
{
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

// expected-note@+1 {{to match this '('}}
ruleset test_table_1 : Table(sensor incubator) // expected-error {{expected ')'}}
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

ruleset test_table_3 : Table(sensor, incubator) Table(sensor, incubator) // expected-error {{Invalid Gaia attribute.}}
{
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

ruleset test_table_4 : Table() { // expected-error {{Invalid Gaia ruleset attribute.}}
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

ruleset test_table_5 : Table("cbcbc") // expected-error {{expected identifier}}
{
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

ruleset test_table_6 : Table(345) // expected-error {{expected identifier}}
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

ruleset test_table_8 : Table(, ) // expected-error {{expected identifier}}
{
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

ruleset test_table_9: Table(sensor, incubator, bogus)     // expected-error {{Table 'bogus' was not found in the catalog.}}
{
    {
        max_temp += min_temp/2;
    }
}

ruleset test_serial_stream_1 : Table(sensor, incubator), SerialStream() // expected-error {{expected identifier}}
{
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}
// expected-note@+1 {{to match this '('}}
ruleset test_serial_stream_2 : Table(sensor, incubator), SerialStream(sdfdf, sfdfsf) // expected-error {{expected ')'}}
{
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

ruleset test_serial_stream_3 : Table(sensor, incubator), SerialStream(, ) // expected-error {{expected identifier}}
{
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

// expected-note@+1 {{to match this '('}}
ruleset test_serial_stream_4 : Table(sensor, incubator), SerialStream(sdsdf, ) // expected-error {{expected ')'}}
{
    {
        min_temp += @value;
        max_temp += min_temp / 2;
    }
}

ruleset test105 Table[actuator] // expected-error {{expected '{'}}
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

ruleset test110 Table{actuator} // expected-error {{expected '{'}}
{ // expected-error {{expected unqualified-id}}
    {
        actuator.value=.5;
    }
}
