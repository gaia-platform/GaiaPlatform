// RUN: %clang_cc1  -fsyntax-only -verify -fgaia-extensions %s

ruleset test  table(sensor, incubator)   // expected-error {{expected '{'}}
{
  OnUpdate(incubator)
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

// expected-note@+1 {{to match this '('}}
ruleset test1: table(sensor incubator)   // expected-error {{expected ')'}}
{
  OnUpdate(incubator)
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test2: table123(sensor, incubator)   // expected-error {{Invalid Gaia attribute.}}
{
  OnUpdate(incubator)
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test3: table(sensor, incubator) table(sensor, incubator)  // expected-error {{Invalid Gaia attribute.}}
{
  OnUpdate(incubator)
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test4: table()
{ // expected-error {{Invalid Gaia attribute.}}
  OnUpdate(incubator)
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test5: table("cbcbc")   // expected-error {{expected identifier}}
{
  OnUpdate(incubator)
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test6: table(345)   // expected-error {{expected identifier}}
{
  OnUpdate(incubator)
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test7:
{ // expected-error {{Invalid Gaia attribute.}}
  OnUpdate(incubator)
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test8: table(,)   // expected-error {{expected identifier}}
{
  OnUpdate(incubator)
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test9
{
  OnUpdate(incubator)
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
    OnUpdate(incubator)
    {
      min_temp+=@value;
      max_temp += min_temp/2;
    }
  }
}

ruleset test12: table(sensor, incubator), SerialStream()  // expected-error {{expected identifier}}
{
  OnUpdate(incubator)
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}
// expected-note@+1 {{to match this '('}}
ruleset test13: table(sensor, incubator), SerialStream(sdfdf,sfdfsf)  // expected-error {{expected ')'}}
{
  OnUpdate(incubator)
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test13: table(sensor, incubator), SerialStream(,)  // expected-error {{expected identifier}}
{
  OnUpdate(incubator)
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}
// expected-note@+1 {{to match this '('}}
ruleset test14: table(sensor, incubator), SerialStream(sdsdf,)  // expected-error {{expected ')'}}
{
  OnUpdate(incubator)
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}

ruleset test15
{
  OnUpdate(incubator)
  {
    min_temp=sensor.LastOperation; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test16
{
  OnUpdate(incubator)
  {
    min_temp=UPDATE; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test17
{
  OnUpdate(incubator)
  {
    min_temp=sensor.LastOperation < UPDATE; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test18
{
  OnUpdate(incubator)
  {
    min_temp=sensor.LastOperation + 3; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test19
{
  OnUpdate(incubator)
  {
    min_temp=UPDATE + 3; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test20
{
  OnUpdate(incubator)
  {
    min_temp = (int) UPDATE ; // expected-error {{Incorrect LastOperation action.}}
  }
}


ruleset test21
{
  OnUpdate(incubator)
  {
    min_temp+= sensor.LastOperation ; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test22
{
  OnUpdate(incubator)
  {
    sensor.LastOperation ++; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test23
{
  OnUpdate(incubator)
  {
    ++UPDATE ; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test24
{
  OnUpdate(incubator)
  {
    min_temp=sensor.LastOperation == 5; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test25
{
  OnUpdate(incubator)
  {
    min_temp = 3 == UPDATE; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test26
{
  OnUpdate(incubator)
  {
    min_temp = !UPDATE; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test27
{
  OnUpdate(incubator)
  {
    min_temp = &sensor.LastOperation; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test28
{
  OnUpdate(incubator)
  {
    bool t = LastOperation == UPDATE; // expected-error {{use of undeclared identifier 'LastOperation'}} expected-error {{Field 'LastOperation' was not found in the catalog.}}
  }
}

ruleset test29
{
  OnUpdate(incubator)
  {
    actuator.value1 ++; // expected-error {{no member named 'value1' in 'actuator__type'; did you mean 'value'?}} expected-note {{'value' declared here}}
  }
}

ruleset test30
{
  OnUpdate(incubator)
  {
    value ++; // expected-error {{Duplicate field 'value'}} expected-error {{use of undeclared identifier 'value'}}
  }
}

ruleset test31
{
  OnUpdate(incubator)
  {
    x.value ++; // expected-error {{Table 'x' was not found in the catalog.}} expected-error {{use of undeclared identifier 'x'}}
  }
}

ruleset test32: table(sensor, incubator, bogus)   // expected-error {{Table 'bogus' was not found in the catalog.}}
{
  OnUpdate(incubator)
  {
    max_temp += min_temp/2;
  }
}

ruleset test33: table(sensor)
{
  OnUpdate(incubator)
  {
    actuator.value += value/2; // expected-warning {{Table 'actuator' is not referenced in table attribute.}}
  }
}

ruleset test34
{
  OnUpdate(incubator)
  {
    auto x = rule_context; // expected-error {{Invalid use of 'rule_context'.}}
  }
}

ruleset test35
{
  OnUpdate(incubator)
  {
    int rule_context = 5; // expected-error {{expected unqualified-id}}
  }
}

ruleset test36
{
  OnUpdate(incubator)
  {
    int ruleset = 5; // expected-error {{expected unqualified-id}}
  }
}

ruleset test37
{
  OnUpdate(incubator)
  {
    auto x = rule_context.x; // expected-error {{no member named 'x' in 'rule_context__type'}}
  }
}

ruleset test38
{
  OnUpdate(incubator)
  {
    auto x = &rule_context; // expected-error {{Invalid use of 'rule_context'.}}
  }
}

ruleset test39
{
  OnUpdate(incubator)
  {
    rule_context.rule_name = "test"; // expected-error {{expression is not assignable}}
  }
}

ruleset test40
{
  OnUpdate(incubator)
  {
    rule_context.rule_name[3] = 't'; // expected-error {{read-only variable is not assignable}}
  }
}

ruleset test41
{
  OnUpdate(incubator)
  {
    rule_context.event_type = 5; // expected-error {{expression is not assignable}}
  }
}

ruleset test42
{
  OnUpdate(incubator)
  {
    rule_context.gaia_type = 5; // expected-error {{expression is not assignable}}
  }
}

ruleset test43
{
  OnUpdate(incubator)
  {
    rule_context.ruleset_name = "test"; // expected-error {{expression is not assignable}}
  }
}

ruleset test44
{
  OnUpdate(incubator)
  {
    rule_context.ruleset_name[3] = 't'; // expected-error {{read-only variable is not assignable}}
  }
}

ruleset test45
{
  OnDelete(incubator) // expected-error {{unknown type name 'OnDelete'}}
  {

  }// expected-error {{expected ';' after top level declarator}}
}

ruleset test46
{
  OnUpdate(tttt) // expected-error {{Field 'tttt' was not found in the catalog.}}
  {

  }
}

ruleset test47
{
  OnUpdate(incubator sensor) // expected-error {{expected ')'}} expected-note {{to match this '('}}
  {

  }
}

ruleset test48
{
  OnUpdate(incubator, sensor.test) // expected-error {{Field 'test' was not found in the catalog.}}
  {

  }
}

ruleset test48
{
  OnUpdate
  { // expected-error {{expected '('}}

  }
}

ruleset test48
{
  OnUpdate()
  { // expected-error {{Invalid Gaia attribute.}}

  }
}

ruleset test49
{
  OnUpdate("sensor") // expected-error {{expected identifier}}
  {

  }
}

ruleset test50
{
  OnUpdate(5) // expected-error {{expected identifier}}
  {

  }
}

ruleset test51
{
  { //expected-error {{expected unqualified-id}}

  }
}

ruleset test52
{
  OnUpdate(value) // expected-error {{Duplicate field 'value'.}}
  {

  }
}

ruleset test53
{
  OnUpdate(incubator),OnUpdate(sensor) // expected-error {{Invalid Gaia rule attribute.}}
  {

  }
}

ruleset test54
{
  OnUpdate(sensor.value.value)// expected-error {{expected ')'}} expected-note {{to match this '('}}
  {

  }
}
