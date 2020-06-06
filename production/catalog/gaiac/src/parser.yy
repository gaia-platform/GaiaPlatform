/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
/*-------------------------------------------------------------------------
 *
 * parser.yy
 *   parser definition for Gaia catalog DDLs
 *
 *-------------------------------------------------------------------------
 */
%skeleton "lalr1.cc" // -*- C++ -*-
%require "3.5"
%defines

%define api.token.raw

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
    #include <string>
    class driver;
    namespace gaia {
    namespace catalog {
    namespace ddl {
        enum class DataType : unsigned int;
        struct Statement;
        struct CreateStatement;
        struct FieldType;
        struct FieldDefinition;
    }
    }
    }
    using namespace gaia::catalog::ddl;
}

// The parsing context.
%param { driver& drv }

%locations

%define parse.trace
%define parse.error verbose

%code {
    #include "driver.hpp"
    #include "gaia_catalog.hpp"
}

%define api.token.prefix {TOK_}
%token BOOLEAN BYTE UBYTE SHORT USHORT INT UINT LONG ULONG FLOAT DOUBLE STRING
%token CREATE TABLE TYPE OF
%token END  0
%token LPAREN "("
%token RPAREN ")"
%token LBRACKET "["
%token RBRACKET "]"
%token COMMA ","
%token DOT "."
%token SEMICOLON ";"

%token <std::string> IDENTIFIER "identifier"
%token <int> NUMBER "number"

%type <Statement*> statement
%type <CreateStatement*> create_statement
%type <FieldType*> field_type
%type <int> opt_array
%type <FieldDefinition*> field_def
%type <std::vector<FieldDefinition*>*> field_def_commalist
%type <std::vector<Statement*>*> statement_list

%printer { yyo << $$; } <*>;

%%
%start input;
input: statement_list opt_semicolon {
    for (Statement* stmt : *$1) {
        drv.statements.push_back(stmt);
    }
};

opt_semicolon: ";" | ;

statement_list:
statement {
    $$ = new std::vector<Statement*>();
    $$->push_back($1); }
| statement_list ";" statement {
    $1->push_back($3);
    $$ = $1;
};

statement: create_statement { $$ = $1; };

create_statement:
    CREATE TABLE IDENTIFIER "(" field_def_commalist ")" {
        $$ = new CreateStatement(CreateType::kCreateTable);
        $$->tableName = std::move($3);
        $$->fields = $5;
    };
    | CREATE TYPE IDENTIFIER "(" field_def_commalist ")" {
        $$ = new CreateStatement(CreateType::kCreateType);
        $$->typeName = std::move($3);
        $$->fields = $5;
    };
    | CREATE TABLE IDENTIFIER OF IDENTIFIER {
        $$ = new CreateStatement(CreateType::kCreateTableOf);
        $$->tableName = std::move($3);
        $$->typeName = std::move($5);
    };

field_def_commalist:
    field_def { $$ = new std::vector<FieldDefinition*>(); $$->push_back($1); }
    | field_def_commalist "," field_def { $1->push_back($3); $$ = $1; };

field_def:
    IDENTIFIER field_type opt_array {
        $$ = new FieldDefinition($1, $2->type, $3);
        if ($$->type == DataType::TABLE) {
           $$->tableTypeName = std::move($2->name);
        }
    };

opt_array:
    "[" "]" { $$ = 0; }
    | "[" NUMBER "]" { $$ = $2; }
    | { $$ = 1; } ;

field_type:
    BOOLEAN { $$ = new FieldType(DataType::BOOLEAN); }
    | BYTE { $$ = new FieldType(DataType::BYTE); }
    | UBYTE { $$ = new FieldType(DataType::UBYTE); }
    | SHORT { $$ = new FieldType(DataType::SHORT); }
    | USHORT { $$ = new FieldType(DataType::USHORT); }
    | INT { $$ = new FieldType(DataType::INT); }
    | UINT { $$ = new FieldType(DataType::UINT); }
    | LONG { $$ = new FieldType(DataType::LONG); }
    | ULONG { $$ = new FieldType(DataType::ULONG); }
    | FLOAT { $$ = new FieldType(DataType::FLOAT); }
    | DOUBLE { $$ = new FieldType(DataType::DOUBLE); }
    | STRING { $$ = new FieldType(DataType::STRING); }
    | IDENTIFIER { $$ = new FieldType(DataType::TABLE); $$->name = $1; };

%%
void
yy::parser::error (const location_type& l, const std::string& m)
{
  std::cerr << l << ": " << m << '\n';
}
