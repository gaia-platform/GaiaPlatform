// RUN: %clang_cc1 -fgaia-extensions -ast-dump %s | FileCheck -strict-whitespace %s

// TODO This is necessary for the connect/disconnect feature, but doesn;t work for that purpose because
//   of this bug: https://gaiaplatform.atlassian.net/browse/GAIAPLAT-1037
ruleset test_link_field
{
    OnInsert(farmer)
    {
        incubators;
        // CHECK: DeclRefExpr 0x{{[^ ]*}} <{{.*}}> 'farmer_incubators__type' lvalue Var 0x{{[^ ]*}} 'incubators' 'farmer_incubators__type'
    }
}
