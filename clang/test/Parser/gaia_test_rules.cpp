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
{// expected-error {{rulesets can not be defined in ruleset scope}}
{
  x+=@z; 
  y += x;
}
}
}

