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
    #include <memory>
    #include <vector>
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
    using field_def_list_t = std::vector<std::unique_ptr<field_definition_t>>;
    using statement_list_t = std::vector<std::unique_ptr<statement_t>>;
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
%token CREATE TABLE REFERENCES
%token END 0
%token LPAREN "("
%token RPAREN ")"
%token LBRACKET "["
%token RBRACKET "]"
%token COMMA ","
%token SEMICOLON ";"

%token <std::string> IDENTIFIER "identifier"
%token <int> NUMBER "number"

%type <std::unique_ptr<statement_t>> statement
%type <std::unique_ptr<create_statement_t>> create_statement
%type <std::unique_ptr<field_type_t>> field_type

%type <int> opt_array
%type <std::unique_ptr<field_definition_t>> field_def
%type <std::unique_ptr<field_def_list_t>> field_def_commalist
%type <std::unique_ptr<statement_list_t>> statement_list

%printer { yyo << "statement"; } statement
%printer { yyo << "create_statement"; } create_statement
%printer { yyo << "filed_type"; } field_type
%printer { yyo << "filed_def:" << $$->name; } field_def
%printer { yyo << "filed_def_commalist[" << $$->size() << "]"; } field_def_commalist
%printer { yyo << "statement_list[" << $$->size() << "]"; } statement_list
%printer { yyo << $$; } <*>

%%
%start input;

input:
    statement_list opt_semicolon {
        gaia_parser.statements = std::move(*$1);
    };

opt_semicolon: ";" | ;

statement_list:
    statement {
        $$ = std::unique_ptr<statement_list_t>{new statement_list_t()};
        $$->push_back(std::move($1)); }
    | statement_list ";" statement {
        $1->push_back(std::move($3));
        $$ = std::move($1);
    };

statement: create_statement { $$ = std::unique_ptr<statement_t>{std::move($1)}; };

create_statement:
    CREATE TABLE IDENTIFIER "(" field_def_commalist ")" {
        $$ = std::unique_ptr<create_statement_t>{new create_statement_t(create_type_t::CREATE_TABLE)};
        $$->table_name = std::move($3);
        $$->fields = std::move(*$5);
    };

field_def_commalist:
    field_def {
        $$ = std::unique_ptr<field_def_list_t>{new field_def_list_t()};
        $$->push_back(std::move($1)); }
    | field_def_commalist "," field_def {
        $1->push_back(std::move($3));
        $$ = std::move($1);
    };

field_def:
    IDENTIFIER field_type opt_array {
        $$ = std::unique_ptr<field_definition_t>{new field_definition_t($1, $2->type, $3)};
        if ($$->type == data_type_t::REFERENCES) {
           $$->table_type_name = std::move($2->name);
        }
    };

opt_array:
    "[" "]" { $$ = 0; }
    | "[" NUMBER "]" { $$ = $2; }
    | { $$ = 1; };

field_type:
    BOOL { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::BOOL)}; }
    | INT8 { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::INT8)}; }
    | UINT8 { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::UINT8)}; }
    | INT16 { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::INT16)}; }
    | UINT16 { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::UINT16)}; }
    | INT32 { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::INT32)}; }
    | UINT32 { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::UINT32)}; }
    | INT64 { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::INT64)}; }
    | UINT64 { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::UINT64)}; }
    | FLOAT32 { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::FLOAT32)}; }
    | FLOAT64 { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::FLOAT64)}; }
    | STRING { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::STRING)}; }
    | REFERENCES IDENTIFIER {
        $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::REFERENCES)};
        $$->name = std::move($2);
    };

%%
void
yy::parser::error (const location_type& l, const std::string& m)
{
  std::cerr << l << ": " << m << '\n';
}
