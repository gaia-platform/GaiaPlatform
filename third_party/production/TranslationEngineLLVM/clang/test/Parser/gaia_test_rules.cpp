// RUN: %clang_cc1  -fsyntax-only -verify -fgaia-extensions %s

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
    min_temp=sensor.LastOperation; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test16
{
  {
    min_temp=UPDATE; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test17
{
  {
    min_temp=sensor.LastOperation < UPDATE; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test18
{
  {
    min_temp=sensor.LastOperation + 3; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test19
{
  {
    min_temp=UPDATE + 3; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test20
{
  {
    min_temp = (int) UPDATE ; // expected-error {{Incorrect LastOperation action.}}
  }
}


ruleset test21
{
  {
    min_temp+= sensor.LastOperation ; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test22
{
  {
    sensor.LastOperation ++; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test23
{
  {
    ++UPDATE ; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test24
{
  {
    min_temp=sensor.LastOperation == 5; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test25
{
  {
    min_temp = 3 == UPDATE; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test26
{
  {
    min_temp = !UPDATE; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test27
{
  {
    min_temp = &sensor.LastOperation; // expected-error {{Incorrect LastOperation action.}}
  }
}

ruleset test28
{
  {
    bool t = LastOperation == UPDATE; // expected-error {{use of undeclared identifier 'LastOperation'}} expected-error {{Field 'LastOperation' was not found in the catalog.}}
  }
}

ruleset test29
{
  {
    actuator.value1 ++; // expected-error {{no member named 'value1' in 'actuator__type'; did you mean 'value'?}} expected-note {{'value' declared here}}
  }
}

ruleset test30
{
  {
    value ++; // expected-error {{Duplicate field 'value'}} expected-error {{use of undeclared identifier 'value'}}
  }
}

ruleset test31
{
  {
    x.value ++; // expected-error {{Table 'x' was not found in the catalog.}} expected-error {{use of undeclared identifier 'x'}}
  }
}

ruleset test32: Table(sensor, incubator, bogus)   // expected-error {{Table 'bogus' was not found in the catalog.}}
{
  {
    max_temp += min_temp/2;
  }
}

ruleset test33: Table(sensor)
{
  {
    actuator.value += value/2; // expected-warning {{Table 'actuator' is not referenced in table attribute.}}
  }
}

ruleset test34
{
  {
    auto x = rule_context; // expected-error {{Invalid use of 'rule_context'.}}
  }
}

ruleset test35
{
  {
    int rule_context = 5; // expected-error {{expected unqualified-id}}
  }
}

ruleset test36
{
  {
    int ruleset = 5; // expected-error {{expected unqualified-id}}
  }
}

ruleset test37
{
  {
    auto x = rule_context.x; // expected-error {{no member named 'x' in 'rule_context__type'}}
  }
}

ruleset test38
{
  {
    auto x = &rule_context; // expected-error {{Invalid use of 'rule_context'.}}
  }
}

ruleset test39
{
  {
    rule_context.rule_name = "test"; // expected-error {{expression is not assignable}}
  }
}

ruleset test40
{
  {
    rule_context.rule_name[3] = 't'; // expected-error {{read-only variable is not assignable}}
  }
}

ruleset test41
{
  {
    rule_context.event_type = 5; // expected-error {{expression is not assignable}}
  }
}

ruleset test42
{
  {
    rule_context.gaia_type = 5; // expected-error {{expression is not assignable}}
  }
}

ruleset test43
{
  {
    rule_context.ruleset_name = "test"; // expected-error {{expression is not assignable}}
  }
}

ruleset test44
{
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
  { // expected-error {{Invalid Gaia rule attribute.}}

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
  OnUpdate(value) // expected-error {{Duplicate field 'value'.}}
  {

  }
}

ruleset test52
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
