// RUN: %clang_cc1  -fsyntax-only -verify -fgaia-extensions %s

// Uncomment the #define to re-test failing tests.
// #define TEST_FAILURES

ruleset test  Table(sensor, incubator)   // expected-error {{expected '{'}}
{
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

// expected-note@+1 {{to match this '('}}
ruleset test1: Table(sensor incubator)   // expected-error {{expected ')'}}
{
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test2: table123(sensor, incubator)   // expected-error {{Invalid Gaia attribute.}}
{
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test3: Table(sensor, incubator) Table(sensor, incubator)  // expected-error {{Invalid Gaia attribute.}}
{
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test4: Table()
{ // expected-error {{Invalid Gaia ruleset attribute.}}
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test5: Table("cbcbc")   // expected-error {{expected identifier}}
{
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test6: Table(345)   // expected-error {{expected identifier}}
{
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test7:
{ // expected-error {{Invalid Gaia attribute.}}
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test8: Table(,)   // expected-error {{expected identifier}}
{
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test9
{
  {
    int value;
    min_temp+=@value;  // expected-error {{unexpected '@' in program}}
    max_temp += min_temp/2;
  }
}

ruleset test10
{
  min_temp+=@value; // expected-error {{unknown type name 'min_temp'}} expected-error {{expected unqualified-id}}
}

{ // expected-error {{expected unqualified-id}}
  min_temp+=@value;
  max_temp += min_temp/2;
}

ruleset test11
{
  ruleset test12
  {// expected-error {{Rulesets can not be defined in ruleset scope.}}
    {
      min_temp+=@value;
      max_temp += min_temp/2;
    }
  }
}

ruleset test12: Table(sensor, incubator), SerialStream()  // expected-error {{expected identifier}}
{
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}
// expected-note@+1 {{to match this '('}}
ruleset test13: Table(sensor, incubator), SerialStream(sdfdf,sfdfsf)  // expected-error {{expected ')'}}
{
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test13: Table(sensor, incubator), SerialStream(,)  // expected-error {{expected identifier}}
{
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}
// expected-note@+1 {{to match this '('}}
ruleset test14: Table(sensor, incubator), SerialStream(sdsdf,)  // expected-error {{expected ')'}}
{
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test15
{
  {
    actuator.value1 ++; // expected-error {{no member named 'value1' in 'actuator__type'; did you mean 'value'?}} expected-note {{'value' declared here}}
  }
}

ruleset test16
{
  {
    value ++; // expected-error {{Duplicate field 'value'}} expected-error {{use of undeclared identifier 'value'}}
  }
}

ruleset test17
{
  {
    x.value ++; // expected-error {{Table 'x' was not found in the catalog.}} expected-error {{use of undeclared identifier 'x'}}
  }
}

ruleset test18: Table(sensor, incubator, bogus)   // expected-error {{Table 'bogus' was not found in the catalog.}}
{
  {
    max_temp += min_temp/2;
  }
}

ruleset test19: Table(sensor)
{
  {
    actuator.value += @value/2; // expected-warning {{Table 'actuator' is not referenced in table attribute.}}
  }
}

ruleset test20
{
  {
    auto x = rule_context; // expected-error {{Invalid use of 'rule_context'.}}
  }
}

ruleset test21
{
  {
    int rule_context = 5; // expected-error {{expected unqualified-id}}
  }
}

ruleset test22
{
  {
    int ruleset = 5; // expected-error {{expected unqualified-id}}
  }
}

ruleset test23
{
  {
    auto x = rule_context.x; // expected-error {{no member named 'x' in 'rule_context__type'}}
  }
}

ruleset test24
{
  {
    auto x = &rule_context; // expected-error {{Invalid use of 'rule_context'.}}
  }
}

ruleset test25
{
  {
    rule_context.rule_name = "test"; // expected-error {{expression is not assignable}}
  }
}

ruleset test26
{
  {
    rule_context.rule_name[3] = 't'; // expected-error {{read-only variable is not assignable}}
  }
}

ruleset test27
{
  {
    rule_context.event_type = 5; // expected-error {{expression is not assignable}}
  }
}

ruleset test28
{
  {
    rule_context.gaia_type = 5; // expected-error {{expression is not assignable}}
  }
}

ruleset test29
{
  {
    rule_context.ruleset_name = "test"; // expected-error {{expression is not assignable}}
  }
}

ruleset test30
{
  {
    rule_context.ruleset_name[3] = 't'; // expected-error {{read-only variable is not assignable}}
  }
}

ruleset test31
{
  OnDelete(incubator) // expected-error {{Invalid Gaia rule attribute.}}
  {

  }
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
  OnUpdate(incubator),OnUpdate(sensor) // expected-error {{Invalid Gaia rule attribute.}}
  {

  }
}

ruleset test53
{
  OnUpdate(sensor.value.value)// expected-error {{expected ')'}} expected-note {{to match this '('}}
  {

  }
}

ruleset test53
{
  OnUpdate(value)// expected-error {{Duplicate field 'value'}}
  {

  }
}

ruleset test54
{
  OnUpdate(sensor.)// expected-error {{expected identifier}}
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
  OnChange(incubator),OnUpdate(sensor) // expected-error {{Invalid Gaia rule attribute.}}
  {

  }
}

ruleset test64
{
  OnChange(sensor.value.value)// expected-error {{expected ')'}} expected-note {{to match this '('}}
  {

  }
}

ruleset test65
{
  OnChange(value)// expected-error {{Duplicate field 'value'}}
  {

  }
}

ruleset test66
{
  OnChange(sensor.)// expected-error {{expected identifier}}
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
  OnInsert
  { // expected-error {{expected '('}}

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
  OnInsert(incubator),OnUpdate(sensor) // expected-error {{Invalid Gaia rule attribute.}}
  {

  }
}

ruleset test76
{
  OnInsert(sensor.value.value)// expected-error {{expected ')'}} expected-note {{to match this '('}}
  {

  }
}

ruleset test77
{
  OnInsert(value)// expected-error {{Duplicate field 'value'}}
  {

  }
}

ruleset test78
{
  OnInsert(sensor.)// expected-error {{expected identifier}}
  {

  }
}

ruleset test79
{
    OnInsert(S:sensor)
    {
        actuator.value += value/2;// expected-error {{Duplicate field 'value'}} expected-error {{use of undeclared identifier 'value'}}
    }
}

ruleset test79 : Table(sensor, actuator)
{
    OnInsert(S:sensor)
    {
        actuator.value += sensor.value/2;
    }
}

ruleset test80
{
    OnInsert(S:sensor)
    {
        actuator.value += S.value/2;
    }
}

ruleset test81
{
    OnUpdate(S:sensor)
    {
        float v = S.value;
    }
}

ruleset test82
{
    OnUpdate(S:sensor, V:sensor.value)
    {
        float v = S.value + V.value;
    }
}

// The 'value' is not duplicated, but qualified by 'sensor'.
// GAIALAT-796
#ifdef TEST_FAILURES
ruleset test83 : Table(sensor)
{
    OnUpdate(value)
    {
        float v;
        v = value * 2.0;
    }
}
#endif

ruleset test84
{
    OnInsert(I::incubator) // expected-error {{expected ')'}} expected-note {{to match this '('}}
    {
    }
}

ruleset test85
{
    OnInsert(I:incubator:sensor) // expected-error {{expected ')'}} expected-note {{to match this '('}})
    {
    }
}

ruleset test86
{
    OnInsert(incubator:) // expected-error {{expected identifier}}
    {
    }
}

ruleset test87
{
    OnInsert(incubator:I) // expected-error {{Tag 'incubator' cannot have the same name as a table or a field.}}
    {
    }
}

ruleset test88
{
    OnUpdate(nonsense, no-sense, not-sensible) // expected-error {{expected ')'}} expected-note {{to match this '('}})
    {
    }
}

ruleset test89 : Table(actuator)
{
    OnUpdate(A:actuator)
    {
    }
}

ruleset test90
{
    OnUpdate(incubator)
    {
        long i = actuator.timestamp;
        long j = /incubator->actuator.timestamp;
    }
}

ruleset test91
{
    OnUpdate(incubator)
    {
        long i = actuator->timestamp; // expected-error {{Tag refers to an invalid table 'timestamp'.}}
                   // expected-error@-1 {{use of undeclared identifier 'actuator'}}
    }
}

ruleset test92
{
    OnUpdate(incubator)
    {
        int i = 0;
        if (/incubator->raised->animal->feeding.portion > 10)
        {
            i += portion;
        }
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
    Onward(incubator) // expected-error {{Invalid Gaia rule attribute.}}
    {
        incubator.min_temp = 0.0;
    }
}

ruleset test95
{
    {
        \incubator.min_temp = 0.0; // expected-error {{expected expression}}
    }
}

ruleset test96
{
    {
        if (farmer-->raised) // expected-error {{cannot decrement value of type 'farmer__type'}}
        {}
    }
}

ruleset test97
{
    {
        if (farmer>-raised) // expected-error {{invalid argument type 'raised__type' to unary expression}}
        {}
    }
}

ruleset test98
{
    {
        if (farmer<-raised) // expected-error {{invalid argument type 'raised__type' to unary expression}}
        {}
    }
}

ruleset test99
{
    {
        if (farmer->yield.bushels)
        {}
    }
}

ruleset test100
{
    {
        float m = / f : farmer -> incubator . min_temp ;
    }
}

ruleset test101
{
    {
        farmer->incubator->raised.birthdate=actuator->incubator->sensor.timestamp; // expected-error {{assigning to 'const char *' from incompatible type 'unsigned long long'}}
    }
}

// GAIAPLAT-913
#ifdef TEST_FAILURES
ruleset test102
{
    {
        animal->feeding->yield=5; // expected-error {{no viable overloaded '='}}
    }
}
#endif

ruleset test103
{
    {
        animal.age>actuator.timestamp; // expected-warning {{relational comparison result unused}}
    }
}

ruleset test104
{
    OnInsert[A:animal] // expected-error {{expected '('}}
    {
        A.age=5;
    }
}

ruleset test105 Table[actuator] // expected-error {{expected '{'}}
{
    {
        actuator.value=.5;
    }
}

ruleset test106
{
    OnInsert(*incubator) { // expected-error {{expected identifier}}
        incubator.min_temp=98.9;
    }
}

ruleset test107
{
    OnInsert(do(min_temp)) { // expected-error {{expected identifier}}
        age=5;
    }
}

ruleset test108
{
    OnInsert(age ? 3 : 4) { // expected-error {{expected ')'}} expected-note {{to match this '('}}
        age=5;
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

// Pathological incorrect syntax cases
ruleset test111 { {.age=5;} } // expected-error {{expected expression}}
ruleset test112 { {->animal.age=5;} } // expected-error {{expected expression}}
ruleset test113 { {animal:.age=5;} } // expected-error {{expected expression}}
ruleset test114 { {animal:=5;} } // expected-error {{expected expression}}
ruleset test115 { {.age=actuator.timestamp;} } // expected-error {{expected expression}}
ruleset test116 { {animal.age=>actuator.timestamp;} } // expected-error {{expected expression}}

#ifdef TEST_FAILURES
ruleset test117 { {animal->.age=5;} } // expected-error {{expected expression}}
ruleset test118 { {3:animal.age=5;} } // expected-error {{expected expression}}
ruleset test119 { {animal[age]=actuator[timestamp];} } // expected-error {{expected expression}}
ruleset test120 { {animal(age)=actuator(timestamp);} } // expected-error {{expected expression}}
ruleset test121 { OnInsert(A:animal) {animal.age=age:A;} }  // expected-error {{expected expression}}
#endif

// GAIAPLAT-827
ruleset test101
{
    OnChange(sensor.value)
    {
        if (S:sensor.value > 100.0)
        {
            actuator.value = 101.0;
        }
    }
}

// GAIAPLAT-808
// The I.min_temp doesn't use the tag from the 'if'.
ruleset testE62
{
    OnChange(I:incubator)
    {
        if (/I:incubator.max_temp == 100.0) // expected-error {{Tag 'I' is already defined.}}
                                            // expected-error@-1  {{use of undeclared identifier 'I'}}
        {
            I.min_temp ++;
        }
    }
}

// GAIAPLAT-808
ruleset testE61
{
    OnChange(actuator)
    {
        if (/I:incubator.max_temp == 100.0)
        {
            I:incubator.min_temp ++; // expected-error {{Tag 'I' is already defined.}}
                                     // expected-error@-1  {{use of undeclared identifier 'incubator'}}
        }

    }
}

// GAIAPLAT-808
ruleset testE6
{
    OnChange(actuator)
    {
        if (/I:incubator.max_temp == 100.0)
        {
            I.min_temp ++;
        }
        I.max_temp++; // expected-error {{Table 'I' was not found in the catalog.}}
                      // expected-error@-1  {{use of undeclared identifier 'I'}}
    }
}

// GAIAPLAT-821
ruleset testE1
{
    OnUpdate(incubator)
    {
        if (/@incubator.min_temp) {
            int i = 0;
        }
    }
}

// GAIAPLAT-821
ruleset testE2
{
    {
        min_temp += @incubator->sensor.value;
    }
}

// GAIAPLAT-822
ruleset testE3
{
    {
        if (farmer->yield.bushels)
        {}
    }
}

// GAIAPLAT-877
#ifdef TEST_FAILURES
ruleset testE4
{
    OnInsert(animal)
    {
        animal->feeding->portion = 5;
    }
}
#endif

// GAIAPLAT-878
// GAIAPLAT-913
#ifdef TEST_FAILURES
ruleset test105
{
    OnInsert(animal)
    {
        animal->feeding = 5;
    }
}
#endif
