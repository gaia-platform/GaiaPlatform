// RUN: %clang_cc1  -fsyntax-only -verify -fgaia-extensions %s

ruleset test  table(sensor, incubator)   // expected-error {{expected '{'}}
{
{
  min_temp+=@value;
  max_temp += min_temp/2;
}
}

// expected-note@+1 {{to match this '('}}
ruleset test1: table(sensor incubator)   // expected-error {{expected ')'}}
{
{
  min_temp+=@value;
  max_temp += min_temp/2;
}
}

ruleset test2: table123(sensor, incubator)   // expected-error {{Invalid ruleset attribute}}
{
{
  min_temp+=@value;
  max_temp += min_temp/2;
}
}

ruleset test3: table(sensor, incubator) table(sensor, incubator)  // expected-error {{Invalid ruleset attribute}}
{
{
  min_temp+=@value;
  max_temp += min_temp/2;
}
}

ruleset test4: table()
{ // expected-error {{Invalid ruleset attribute}}
{
  min_temp+=@value;
  max_temp += min_temp/2;
}
}

ruleset test5: table("cbcbc")   // expected-error {{expected identifier}}
{
{
  min_temp+=@value;
  max_temp += min_temp/2;
}
}

ruleset test6: table(345)   // expected-error {{expected identifier}}
{
{
  min_temp+=@value;
  max_temp += min_temp/2;
}
}

ruleset test7:
{ // expected-error {{Invalid ruleset attribute}}
{
  min_temp+=@value;
  max_temp += min_temp/2;
}
}

ruleset test8: table(,)   // expected-error {{expected identifier}}
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
{// expected-error {{Rulesets can not be defined in ruleset scope}}
{
  min_temp+=@value;
  max_temp += min_temp/2;
}
}
}

ruleset test12: table(sensor, incubator), SerialStream()  // expected-error {{expected identifier}}
{
{
  min_temp+=@value;
  max_temp += min_temp/2;
}
}
// expected-note@+1 {{to match this '('}}
ruleset test13: table(sensor, incubator), SerialStream(sdfdf,sfdfsf)  // expected-error {{expected ')'}}
{
{
  min_temp+=@value;
  max_temp += min_temp/2;
}
}

ruleset test13: table(sensor, incubator), SerialStream(,)  // expected-error {{expected identifier}}
{
{
  min_temp+=@value;
  max_temp += min_temp/2;
}
}
// expected-note@+1 {{to match this '('}}
ruleset test14: table(sensor, incubator), SerialStream(sdsdf,)  // expected-error {{expected ')'}}
{
{
  min_temp+=@value;
  max_temp += min_temp/2;
}
}

ruleset test15
{
{
  min_temp=sensor.LastOperation; // expected-error {{Incorrect LastOperation action}}
}
}

ruleset test16
{
{
  min_temp=UPDATE; // expected-error {{Incorrect LastOperation action}}
}
}

ruleset test17
{
{
  min_temp=sensor.LastOperation < UPDATE; // expected-error {{Incorrect LastOperation action}}
}
}

ruleset test18
{
{
  min_temp=sensor.LastOperation + 3; // expected-error {{Incorrect LastOperation action}}
}
}

ruleset test19
{
{
  min_temp=UPDATE + 3; // expected-error {{Incorrect LastOperation action}}
}
}

ruleset test20
{
{
  min_temp = (int) UPDATE ; // expected-error {{Incorrect LastOperation action}}
}
}


ruleset test21
{
{
  min_temp+= sensor.LastOperation ; // expected-error {{Incorrect LastOperation action}}
}
}

ruleset test22
{
{
  sensor.LastOperation ++; // expected-error {{Incorrect LastOperation action}}
}
}

ruleset test23
{
{
  ++UPDATE ; // expected-error {{Incorrect LastOperation action}}
}
}

ruleset test24
{
{
  min_temp=sensor.LastOperation == 5; // expected-error {{Incorrect LastOperation action}}
}
}

ruleset test25
{
{
  min_temp = 3 == UPDATE; // expected-error {{Incorrect LastOperation action}}
}
}

ruleset test26
{
{
  min_temp = !UPDATE; // expected-error {{Incorrect LastOperation action}}
}
}

ruleset test27
{
{
  min_temp = &sensor.LastOperation; // expected-error {{Incorrect LastOperation action}}
}
}

ruleset test28
{
{
  bool t = LastOperation == UPDATE; // expected-error {{use of undeclared identifier 'LastOperation'}} expected-error {{Field LastOperation was not found in the catalog}}
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
  value ++; // expected-error {{Duplicate field value}} expected-error {{use of undeclared identifier 'value'}}
}
}

ruleset test31
{
{
  x.value ++; // expected-error {{No table x was found in the catalog}} expected-error {{use of undeclared identifier 'x'}}
}
}

ruleset test32: table(sensor, incubator, bogus)   // expected-error {{No table bogus was found in the catalog}}
{
{
  max_temp += min_temp/2;
}
}

ruleset test33: table(bogus, sensor, incubator)   // expected-error {{No table bogus was found in the catalog}}
{
{
  max_temp += min_temp/2;
}
}
