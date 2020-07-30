// RUN: %clang_cc1 -fgaia-extensions -ast-dump %s | FileCheck -strict-whitespace %s

ruleset test : table(sensor, incubator), SerialStream(ttt)
{

{
  min_temp+=@value; 
  max_temp += min_temp/2;
}
}
// CHECK:      RulesetDecl{{.*}} test
// CHECK-NEXT:     RulesetTableAttr 0x{{[^ ]*}} <col:16, col:39> 0x{{[^ ]*}} 0x{{[^ ]*}}
// CHECK-NEXT:     -SerialStreamAttr 0x{{[^ ]*}} <col:42, col:58> ttt
// CHECK:      FunctionDecl{{.*}} {{.*}} 'void (...)' 
// CHECK:     DeclRefExpr 0x{{[^ ]*}} <col:3> 'float' lvalue Var 0x{{[^ ]*}} 'min_temp' 'float'
// CHECK:     DeclRefExpr 0x{{[^ ]*}} <col:14> 'float' lvalue Var 0x{{[^ ]*}} 'value' 'float'
// CHECK:     DeclRefExpr 0x{{[^ ]*}} <col:3> 'float' lvalue Var 0x{{[^ ]*}} 'max_temp' 'float'
// CHECK:     DeclRefExpr 0x{{[^ ]*}} <col:15> 'float' lvalue Var 0x{{[^ ]*}} 'min_temp' 'float'
// CHECK:     RuleAttr 0x{{[^ ]*}} <line:6:1> 


ruleset test1 
{

{
  incubator.min_temp+=@sensor.value; 
  incubator.max_temp += incubator.min_temp/2;
}
}
// CHECK:      RulesetDecl{{.*}} test1
// CHECK:      FunctionDecl{{.*}} {{.*}} 'void (...)' 
// CHECK:     MemberExpr 0x{{[^ ]*}} <col:3, col:13> 'float' lvalue .min_temp 0x{{[^ ]*}}
// CHECK-NEXT:     DeclRefExpr 0x{{[^ ]*}} <col:3> 'incubator__type' lvalue Var 0x{{[^ ]*}} 'incubator' 'incubator__type'
// CHECK:     MemberExpr 0x{{[^ ]*}} <col:24, col:31> 'float' lvalue .value 0x{{[^ ]*}}
// CHECK-NEXT:     DeclRefExpr 0x{{[^ ]*}} <col:24> 'sensor__type' lvalue Var 0x{{[^ ]*}} 'sensor' 'sensor__type'
// CHECK:     MemberExpr 0x{{[^ ]*}} <col:3, col:13> 'float' lvalue .max_temp 0x{{[^ ]*}}
// CHECK-NEXT:     DeclRefExpr 0x{{[^ ]*}} <col:3> 'incubator__type' lvalue Var 0x{{[^ ]*}} 'incubator' 'incubator__type'
// CHECK:     MemberExpr 0x{{[^ ]*}} <col:25, col:35> 'float' lvalue .min_temp 0x{{[^ ]*}}
// CHECK-NEXT:     DeclRefExpr 0x{{[^ ]*}} <col:25> 'incubator__type' lvalue Var 0x{{[^ ]*}} 'incubator' 'incubator__type'
// CHECK:     RuleAttr 0x{{[^ ]*}} <line:25:1>

