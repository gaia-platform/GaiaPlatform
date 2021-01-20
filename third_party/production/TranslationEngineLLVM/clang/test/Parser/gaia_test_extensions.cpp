// RUN: %clang_cc1 -fgaia-extensions -ast-dump %s | FileCheck -strict-whitespace %s

ruleset test : table(sensor, incubator), SerialStream(ttt)
{

{
  min_temp+=@value;
  max_temp += min_temp/2;
  switch (sensor.LastOperation)
  {
  	case DELETE:
  		break;
  	case UPDATE:
  		break;
  	case INSERT:
  		break;
  	case NONE:
  		break;
  	default:
  		break;
  }
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
// CHECK:     MemberExpr 0x{{[^ ]*}} <col:11, col:18> 'int' lvalue .LastOperation 0x{{[^ ]*}}
// CHECK:     DeclRefExpr 0x{{[^ ]*}} <col:9> 'const int' lvalue Var 0x{{[^ ]*}} 'DELETE' 'const int'
// CHECK:     DeclRefExpr 0x{{[^ ]*}} <col:9> 'const int' lvalue Var 0x{{[^ ]*}} 'UPDATE' 'const int'
// CHECK:     DeclRefExpr 0x{{[^ ]*}} <col:9> 'const int' lvalue Var 0x{{[^ ]*}} 'INSERT' 'const int'
// CHECK:     DeclRefExpr 0x{{[^ ]*}} <col:9> 'const int' lvalue Var 0x{{[^ ]*}} 'NONE' 'const int'
// CHECK:     RuleAttr 0x{{[^ ]*}} <line:6:1>


ruleset test1
{

{
  incubator.min_temp+=@sensor.value;
  incubator.max_temp += incubator.min_temp/2;
  if (incubator.LastOperation == DELETE)
  {
  }
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
// CHECK:     MemberExpr 0x{{[^ ]*}} <col:7, col:17> 'int' lvalue .LastOperation 0x{{[^ ]*}}
// CHECK:     DeclRefExpr 0x{{[^ ]*}} <col:34> 'const int' lvalue Var 0x{{[^ ]*}} 'DELETE' 'const int'
// CHECK:     RuleAttr 0x{{[^ ]*}} <line:43:1>


typedef enum
{
    undefined = 0,
    actuators = 1,
    sensors = 2,
    incubators = 3
} hardware_type;

ruleset test2
{
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
// CHECK:     MemberExpr 0x{{[^ ]*}} <col:5, col:14> 'float' lvalue .value 0x{{[^ ]*}}
// CHECK-NEXT:     DeclRefExpr 0x{{[^ ]*}} <col:5> 'actuator__type' lvalue Var 0x{{[^ ]*}} 'actuator' 'actuator__type'
// CHECK:     RuleAttr 0x{{[^ ]*}} <line:76:3>


typedef enum
{
    values = 0,
    types = 1,
    defs = 2
} testEnum;

ruleset test3 : table (sensor)
{
  {
	  if (value < 5)
	  {
		  value = 5;
	  }
  }
}

ruleset test4
{
  {
    auto x = rule_context.rule_id;
    auto y = rule_context.rule_ruleset;
    auto e = rule_context.rule_event_type;
    auto g = rule_context.rule_gaia_type;
  }
}

// CHECK:   RulesetDecl {{.*}} test4
// CHECK:   FunctionDecl{{.*}} {{.*}} 'void (...)'
// CHECK:   VarDecl 0x{{[^ ]*}} <col:5, col:27> col:10 x 'const char *':'const char *' cinit
// CHECK:   MemberExpr 0x{{[^ ]*}} <col:14, col:27> 'const char *' xvalue .rule_id 0x{{[^ ]*}}
// CHECK:   GaiaRuleContextExpr 0x{{[^ ]*}} <col:14> 'rule_context__type' rule_context
// CHECK:   VarDecl 0x{{[^ ]*}} <col:5, col:27> col:10 y 'const char *':'const char *' cinit
// CHECK:   MemberExpr 0x{{[^ ]*}} <col:14, col:27> 'const char *' xvalue .rule_ruleset 0x{{[^ ]*}}
// CHECK:   GaiaRuleContextExpr 0x{{[^ ]*}} <col:14> 'rule_context__type' rule_context
// CHECK:   VarDecl 0x{{[^ ]*}} <col:5, col:27> col:10 e 'unsigned int':'unsigned int' cinit
// CHECK:   MemberExpr 0x{{[^ ]*}} <col:14, col:27> 'const unsigned int' xvalue .rule_event_type 0x{{[^ ]*}}
// CHECK:   GaiaRuleContextExpr 0x{{[^ ]*}} <col:14> 'rule_context__type' rule_context
// CHECK:   VarDecl 0x{{[^ ]*}} <col:5, col:27> col:10 g 'unsigned int':'unsigned int' cinit
// CHECK:   MemberExpr 0x{{[^ ]*}} <col:14, col:27> 'const unsigned int' xvalue .rule_gaia_type 0x{{[^ ]*}}
// CHECK:   GaiaRuleContextExpr 0x{{[^ ]*}} <col:14> 'rule_context__type' rule_context
