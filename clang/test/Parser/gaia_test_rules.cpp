// RUN: %clang_cc1  -fsyntax-only -verify -fgaia-extensions %s

ruleset test  table(dfsdf,sdfsdf,sfsdf)   // expected-error {{expected '{'}}
{
{
  x+=@z; 
  y += x;
}
}

// expected-note@+1 {{to match this '('}}
ruleset test1: table(dfsdf sdfsdf,sfsdf)   // expected-error {{expected ')'}} 
{
{
  x+=@z; 
  y += x;
}
}

ruleset test2: table123(dfsdf,sdfsdf,sfsdf)   // expected-error {{Invalid ruleset attribute}} 
{
{
  x+=@z; 
  y += x;
}
}

ruleset test3: table(dfsdf,sdfsdf,sfsdf) table(dfsdf,sdfsdf,sfsdf)  // expected-error {{Invalid ruleset attribute}} 
{
{
  x+=@z; 
  y += x;
}
}

ruleset test4: table()  
{ // expected-error {{Invalid ruleset attribute}} 
{
  x+=@z; 
  y += x;
}
}

ruleset test5: table("cbcbc")   // expected-error {{expected identifier}} 
{ 
{
  x+=@z; 
  y += x;
}
}

ruleset test6: table(345)   // expected-error {{expected identifier}} 
{ 
{
  x+=@z; 
  y += x;
}
}

ruleset test7:    
{ // expected-error {{Invalid ruleset attribute}} 
{
  x+=@z; 
  y += x;
}
}

ruleset test8: table(,)   // expected-error {{expected identifier}} 
{ 
{
  x+=@z; 
  y += x;
}
}

ruleset test9   
{ 
{
  int z;
  x+=@z; // expected-error {{unexpected '@' in program}} 
  y += x;
}
}

ruleset test10    
{ 

  x+=@z; // expected-error {{unknown type name 'x'}} expected-error {{expected unqualified-id}} 
}

{ // expected-error {{expected unqualified-id}}
  x+=@z; 
  y += x;
}

ruleset test11    
{ 
ruleset test12  
{// expected-error {{Rulesets can not be defined in ruleset scope}}
{
  x+=@z; 
  y += x;
}
}
}

ruleset test12: table(dfsdf,sdfsdf,sfsdf), SerialStream()  // expected-error {{expected identifier}} 
{
{
  x+=@z; 
  y += x;
}
}
// expected-note@+1 {{to match this '('}}
ruleset test13: table(dfsdf,sdfsdf,sfsdf), SerialStream(sdfdf,sfdfsf)  // expected-error {{expected ')'}} 
{
{
  x+=@z; 
  y += x;
}
}

ruleset test13: table(dfsdf,sdfsdf,sfsdf), SerialStream(,)  // expected-error {{expected identifier}} 
{
{
  x+=@z; 
  y += x;
}
}
// expected-note@+1 {{to match this '('}}
ruleset test14: table(dfsdf,sdfsdf,sfsdf), SerialStream(sdsdf,)  // expected-error {{expected ')'}} 
{
{
  x+=@z; 
  y += x;
}
}

ruleset test14  
{
{
  y=x.LastOperation; // expected-error {{Incorrect LastOperation operation}} 
}
}

ruleset test15   
{
{
  y=UPDATE; // expected-error {{Incorrect LastOperation operation}} 
}
}

ruleset test16 
{
{
  y=x.LastOperation < UPDATE; // expected-error {{Incorrect LastOperation operation}} 
}
}

ruleset test17 
{
{
  y=x.LastOperation + 3; // expected-error {{Incorrect LastOperation operation}} 
}
}

ruleset test18   
{
{
  y=UPDATE + 3; // expected-error {{Incorrect LastOperation operation}} 
}
}

ruleset test19	
{
{
  y= (int) UPDATE ; // expected-error {{Incorrect LastOperation operation}} 
}
}


ruleset test21
{
{
  y+= x.LastOperation ; // expected-error {{Incorrect LastOperation operation}} 
}
}

ruleset test22 
{
{
  x.LastOperation ++; // expected-error {{Incorrect LastOperation operation}} 
}
}

ruleset test21
{
{
  ++UPDATE ; // expected-error {{Incorrect LastOperation operation}} 
}
}

ruleset test22 
{
{
  y=x.LastOperation == 5; // expected-error {{Incorrect LastOperation operation}} 
}
}

ruleset test23  
{
{
  y= 3 == UPDATE; // expected-error {{Incorrect LastOperation operation}} 
}
}

ruleset test24  
{
{
  y= !UPDATE; // expected-error {{Incorrect LastOperation operation}} 
}
}

ruleset test24  
{
{
  y= &x.LastOperation; // expected-error {{Incorrect LastOperation operation}} 
}
}
