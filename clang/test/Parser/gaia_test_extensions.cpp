// RUN: %clang_cc1 -fgaia-extensions -ast-dump %s | FileCheck -strict-whitespace %s

ruleset test : table(dfsdf,sdfsdf,sfsdf), SerialStream(ttt)
{

{
  x+=@z; 
  y += x;
}
}
// CHECK:      RulesetDecl{{.*}} test
// CHECK-NEXT:     TableAttr 0x{{[^ ]*}} <col:16, col:40> 0x{{[^ ]*}} 0x{{[^ ]*}} 0x{{[^ ]*}}
// CHECK-NEXT:     -SerialStreamAttr 0x{{[^ ]*}} <col:43, col:59> ttt
// CHECK:      FunctionDecl{{.*}} {{.*}} 'void (...)' 
// CHECK:     DeclRefExpr 0x{{[^ ]*}} <col:3> 'unsigned int' lvalue Var 0x{{[^ ]*}} 'x' 'unsigned int'
// CHECK:     DeclRefExpr 0x{{[^ ]*}} <col:7> 'unsigned int' lvalue Var 0x{{[^ ]*}} 'z' 'unsigned int'
// CHECK:     DeclRefExpr 0x{{[^ ]*}} <col:3> 'unsigned int' lvalue Var 0x{{[^ ]*}} 'y' 'unsigned int'
// CHECK:     DeclRefExpr 0x{{[^ ]*}} <col:8> 'unsigned int' lvalue Var 0x{{[^ ]*}} 'x' 'unsigned int'
// CHECK:     RuleAttr 0x{{[^ ]*}} <line:6:1> 

