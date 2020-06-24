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
    namespace gaia {
    namespace catalog {
    namespace ddl {
        enum class data_type_t : unsigned int;
        struct statement_t;
        struct create_statement_t;
        struct field_type_t;
        struct field_definition_t;
        class parser_t;
    }
    }
    }

    using namespace gaia::catalog::ddl;
}

// The parsing context.
%param { gaia::catalog::ddl::parser_t& gaia_parser }

%locations

%define parse.trace
%define parse.error verbose

%code {
    #include "gaia_parser.hpp"
    #include "gaia_catalog.hpp"
    yy::parser::symbol_type yylex(gaia::catalog::ddl::parser_t &);
}

%define api.token.prefix {TOK_}
%token BOOL INT8 UINT8 INT16 UINT16 INT32 UINT32 INT64 UINT64 FLOAT32 FLOAT64 STRING
%token CREATE TABLE
%token END  0
%token LPAREN "("
%token RPAREN ")"
%token LBRACKET "["
%token RBRACKET "]"
%token COMMA ","
%token SEMICOLON ";"

%token <std::string> IDENTIFIER "identifier"
%token <int> NUMBER "number"

%type <statement_t*> statement
%type <create_statement_t*> create_statement
%type <field_type_t*> field_type

%type <int> opt_array
%type <field_definition_t*> field_def
%type <std::vector<field_definition_t*>*> field_def_commalist
%type <std::vector<statement_t*>*> statement_list

%printer { yyo << $$; } <*>;

%%
%start input;
input: statement_list opt_semicolon {
    for (statement_t* stmt : *$1) {
        gaia_parser.statements.push_back(stmt);
    }
};

opt_semicolon: ";" | ;

statement_list:
statement {
    $$ = new std::vector<statement_t*>();
    $$->push_back($1); }
| statement_list ";" statement {
    $1->push_back($3);
    $$ = $1;
};

statement: create_statement { $$ = $1; };

create_statement:
    CREATE TABLE IDENTIFIER "(" field_def_commalist ")" {
        $$ = new create_statement_t(create_type_t::CREATE_TABLE);
        $$->tableName = std::move($3);
        $$->fields = $5;
    };

field_def_commalist:
    field_def { $$ = new std::vector<field_definition_t*>(); $$->push_back($1); }
    | field_def_commalist "," field_def { $1->push_back($3); $$ = $1; };

field_def:
    IDENTIFIER field_type opt_array {
        $$ = new field_definition_t($1, $2->type, $3);
        if ($$->type == data_type_t::TABLE) {
           $$->table_type_name = std::move($2->name);
        }
    };

opt_array:
    "[" "]" { $$ = 0; }
    | "[" NUMBER "]" { $$ = $2; }
    | { $$ = 1; } ;

field_type:
    BOOL{ $$ = new field_type_t(data_type_t::BOOL); }
    | INT8 { $$ = new field_type_t(data_type_t::INT8); }
    | UINT8 { $$ = new field_type_t(data_type_t::UINT8); }
    | INT16 { $$ = new field_type_t(data_type_t::INT16); }
    | UINT16 { $$ = new field_type_t(data_type_t::UINT16); }
    | INT32 { $$ = new field_type_t(data_type_t::INT32); }
    | UINT32 { $$ = new field_type_t(data_type_t::UINT32); }
    | INT64 { $$ = new field_type_t(data_type_t::INT64); }
    | UINT64 { $$ = new field_type_t(data_type_t::UINT64); }
    | FLOAT32 { $$ = new field_type_t(data_type_t::FLOAT32); }
    | FLOAT64 { $$ = new field_type_t(data_type_t::FLOAT64); }
    | STRING { $$ = new field_type_t(data_type_t::STRING); }
    | IDENTIFIER { $$ = new field_type_t(data_type_t::TABLE); $$->name = $1; };

%%
void
yy::parser::error (const location_type& l, const std::string& m)
{
  std::cerr << l << ": " << m << '\n';
}
