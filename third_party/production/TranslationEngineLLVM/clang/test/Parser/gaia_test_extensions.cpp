// RUN: %clang_cc1 -fgaia-extensions -ast-dump %s | FileCheck -strict-whitespace %s

ruleset test : Table(sensor, incubator), SerialStream(ttt)
{
  OnUpdate(incubator, sensor.value)
  {
    min_temp+=@value;
    max_temp += min_temp/2;
  }
}
// CHECK:      RulesetDecl{{.*}} test
// CHECK-NEXT:     RulesetTableAttr 0x{{[^ ]*}} <col:16, col:39> 0x{{[^ ]*}} 0x{{[^ ]*}}
// CHECK-NEXT:     -SerialStreamAttr 0x{{[^ ]*}} <col:42, col:58> ttt
// CHECK:      FunctionDecl{{.*}} {{.*}} 'void (...)'
// CHECK:     DeclRefExpr 0x{{[^ ]*}} <col:5> 'float' lvalue Var 0x{{[^ ]*}} 'min_temp' 'float'
// CHECK:     DeclRefExpr 0x{{[^ ]*}} <col:16> 'float' lvalue Var 0x{{[^ ]*}} 'value' 'float'
// CHECK:     DeclRefExpr 0x{{[^ ]*}} <col:5> 'float' lvalue Var 0x{{[^ ]*}} 'max_temp' 'float'
// CHECK:     DeclRefExpr 0x{{[^ ]*}} <col:17> 'float' lvalue Var 0x{{[^ ]*}} 'min_temp' 'float'
// CHECK:     GaiaOnUpdateAttr 0x{{[^ ]*}} <line:5:3, col:35> incubator sensor
// CHECK:     RuleAttr 0x{{[^ ]*}} <line:6:3>


ruleset test1
{
  OnInsert(incubator)
  {
    incubator.min_temp +=@sensor.value;
    incubator.max_temp += incubator.min_temp/2;
  }
}
// CHECK:      RulesetDecl{{.*}} test1
// CHECK:      FunctionDecl{{.*}} {{.*}} 'void (...)'
// CHECK:     MemberExpr 0x{{[^ ]*}} <col:5, col:15> 'float' lvalue .min_temp 0x{{[^ ]*}}
// CHECK-NEXT:     DeclRefExpr 0x{{[^ ]*}} <col:5> 'incubator__type' lvalue Var 0x{{[^ ]*}} 'incubator' 'incubator__type'
// CHECK:     MemberExpr 0x{{[^ ]*}} <col:27, col:34> 'float' lvalue .value 0x{{[^ ]*}}
// CHECK-NEXT:     DeclRefExpr 0x{{[^ ]*}} <col:27> 'sensor__type' lvalue Var 0x{{[^ ]*}} 'sensor' 'sensor__type'
// CHECK:     MemberExpr 0x{{[^ ]*}} <col:5, col:15> 'float' lvalue .max_temp 0x{{[^ ]*}}
// CHECK-NEXT:     DeclRefExpr 0x{{[^ ]*}} <col:5> 'incubator__type' lvalue Var 0x{{[^ ]*}} 'incubator' 'incubator__type'
// CHECK:     MemberExpr 0x{{[^ ]*}} <col:27, col:37> 'float' lvalue .min_temp 0x{{[^ ]*}}
// CHECK-NEXT:     DeclRefExpr 0x{{[^ ]*}} <col:27> 'incubator__type' lvalue Var 0x{{[^ ]*}} 'incubator' 'incubator__type'
// CHECK:     GaiaOnInsertAttr 0x{{[^ ]*}} <line:25:3, col:21> incubator
// CHECK:     RuleAttr 0x{{[^ ]*}} <line:26:3>


typedef enum
{
    undefined = 0,
    actuators = 1,
    sensors = 2,
    incubators = 3
} hardware_type;

ruleset test2
{
  OnChange(actuator)
  {
   if (actuator.value < 5)
    {
      actuator.value = 5;
    }
  }
}

// CHECK:      RulesetDecl{{.*}} test2
// CHECK:      FunctionDecl{{.*}} {{.*}} 'void (...)'
// CHECK:     MemberExpr 0x{{[^ ]*}} <col:8, col:17> 'float' lvalue .value 0x{{[^ ]*}}
// CHECK-NEXT:     DeclRefExpr 0x{{[^ ]*}} <col:8> 'actuator__type' lvalue Var 0x{{[^ ]*}} 'actuator' 'actuator__type'
// CHECK:     MemberExpr 0x{{[^ ]*}} <col:7, col:16> 'float' lvalue .value 0x{{[^ ]*}}
// CHECK-NEXT:     DeclRefExpr 0x{{[^ ]*}} <col:7> 'actuator__type' lvalue Var 0x{{[^ ]*}} 'actuator' 'actuator__type'
// CHECK:     RuleAttr 0x{{[^ ]*}} <line:56:3>


typedef enum
{
    values = 0,
    types = 1,
    defs = 2
} testEnum;

ruleset test3 : Table (sensor)
{
  {
    if (@value < 5)
    {
      value = 5;
    }
  }
}

ruleset test4
{
  {
    auto x = rule_context.rule_name;
    auto y = rule_context.ruleset_name;
    auto e = rule_context.event_type;
    auto g = rule_context.gaia_type;
  }
}

// CHECK:   RulesetDecl {{.*}} test4
// CHECK:   FunctionDecl{{.*}} {{.*}} 'void (...)'
// CHECK:   VarDecl 0x{{[^ ]*}} <col:5, col:27> col:10 x 'const char *':'const char *' cinit
// CHECK:   MemberExpr 0x{{[^ ]*}} <col:14, col:27> 'const char *' xvalue .rule_name 0x{{[^ ]*}}
// CHECK:   GaiaRuleContextExpr 0x{{[^ ]*}} <col:14> 'rule_context__type' rule_context
// CHECK:   VarDecl 0x{{[^ ]*}} <col:5, col:27> col:10 y 'const char *':'const char *' cinit
// CHECK:   MemberExpr 0x{{[^ ]*}} <col:14, col:27> 'const char *' xvalue .ruleset_name 0x{{[^ ]*}}
// CHECK:   VarDecl 0x{{[^ ]*}} <col:5, col:27> col:10 e 'unsigned int':'unsigned int' cinit
// CHECK:   MemberExpr 0x{{[^ ]*}} <col:14, col:27> 'const unsigned int' xvalue .event_type 0x{{[^ ]*}}
// CHECK:   GaiaRuleContextExpr 0x{{[^ ]*}} <col:14> 'rule_context__type' rule_context
// CHECK:   VarDecl 0x{{[^ ]*}} <col:5, col:27> col:10 g 'unsigned int':'unsigned int' cinit
// CHECK:   MemberExpr 0x{{[^ ]*}} <col:14, col:27> 'const unsigned int' xvalue .gaia_type 0x{{[^ ]*}}
// CHECK:   GaiaRuleContextExpr 0x{{[^ ]*}} <col:14> 'rule_context__type' rule_context

ruleset test5
{
  OnChange(a:actuator)
  {
    if (actuator.value < 5)
    {
      actuator.value = 5;
    }
  }
}

// CHECK:      RulesetDecl{{.*}} test5
// CHECK:      FunctionDecl{{.*}} {{.*}} 'void (...)'
// CHECK:     MemberExpr 0x{{[^ ]*}} <col:9, col:18> 'float' lvalue .value 0x{{[^ ]*}}
// CHECK-NEXT:     DeclRefExpr 0x{{[^ ]*}} <col:9> 'actuator__type' lvalue Var 0x{{[^ ]*}} 'actuator' 'actuator__type'
// CHECK:     MemberExpr 0x{{[^ ]*}} <col:7, col:16> 'float' lvalue .value 0x{{[^ ]*}}
// CHECK-NEXT:     DeclRefExpr 0x{{[^ ]*}} <col:7> 'actuator__type' lvalue Var 0x{{[^ ]*}} 'actuator' 'actuator__type'
// CHECK:     RuleAttr 0x{{[^ ]*}} <line:117:3>
