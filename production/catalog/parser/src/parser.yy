/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
/**
 * Gaia data definition language (DDL) Bison input file
 *
 * Coding style for this file:
 * - The braced C++ code follows "Gaia C++ Coding Guidelines".
 * - The grammar rule definition loosely follows the style used in the Bison (3.5) manual.
 **/
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
    enum class data_type_t : uint8_t;
    namespace ddl {
        struct statement_t;
        struct create_statement_t;
        struct drop_statement_t;
        struct field_type_t;
        struct field_definition_t;
        class parser_t;
    } // namespace ddl
    } // namespace catalog
    } // namespace gaia

    using field_def_list_t = std::vector<std::unique_ptr<gaia::catalog::ddl::field_definition_t>>;
    using statement_list_t = std::vector<std::unique_ptr<gaia::catalog::ddl::statement_t>>;
    using data_type_t = gaia::catalog::data_type_t;
    using composite_name_t = std::pair<std::string, std::string>;
}

// The parsing context.
%param { gaia::catalog::ddl::parser_t& gaia_parser }

%locations

%define parse.trace
%define parse.error verbose

%code {
    #include "gaia_parser.hpp"
    #include "gaia_catalog.hpp"

    using namespace gaia::catalog::ddl;
    yy::parser::symbol_type yylex(parser_t &);
}

%define api.token.prefix {TOK_}

%token BOOL INT8 UINT8 INT16 UINT16 INT32 UINT32 INT64 UINT64 FLOAT DOUBLE STRING
%token CREATE DROP DATABASE TABLE IF NOT EXISTS REFERENCES ACTIVE
%token END 0
%token LPAREN "("
%token RPAREN ")"
%token LBRACKET "["
%token RBRACKET "]"
%token COMMA ","
%token DOT "."
%token SEMICOLON ";"

%token <std::string> IDENTIFIER "identifier"
%token <int> NUMBER "number"

%type <std::unique_ptr<gaia::catalog::ddl::statement_t>> statement
%type <std::unique_ptr<gaia::catalog::ddl::create_statement_t>> create_statement
%type <std::unique_ptr<gaia::catalog::ddl::drop_statement_t>> drop_statement
%type <std::unique_ptr<gaia::catalog::ddl::field_type_t>> field_type

%type <int> opt_array
%type <bool> opt_if_not_exists
%type <std::unique_ptr<gaia::catalog::ddl::field_definition_t>> field_def
%type <std::unique_ptr<field_def_list_t>> field_def_commalist
%type <std::unique_ptr<statement_list_t>> statement_list
%type <composite_name_t> composite_name

%printer { yyo << "statement"; } statement
%printer { yyo << "create_statement:" << $$->name; } create_statement
%printer { yyo << "drop_statement:" << $$->name; } drop_statement
%printer { yyo << "filed_type"; } field_type
%printer { yyo << "filed_def:" << $$->name; } field_def
%printer { yyo << "filed_def_commalist[" << $$->size() << "]"; } field_def_commalist
%printer { yyo << "statement_list[" << $$->size() << "]"; } statement_list
%printer { yyo << "composite_name: " << $$.first << "." << $$.second; } composite_name
%printer { yyo << $$; } <*>

%%
%start input;

input:
  statement_list opt_semicolon {
      gaia_parser.statements = std::move(*$1);
  }
;

opt_semicolon: ";" | ;

statement_list:
  statement {
      $$ = std::unique_ptr<statement_list_t>{new statement_list_t()};
      $$->push_back(std::move($1));
  }
| statement_list ";" statement {
      $1->push_back(std::move($3));
      $$ = std::move($1);
  }
;

opt_if_not_exists: IF NOT EXISTS { $$ = true; } | { $$ = false; };

statement:
  create_statement { $$ = std::unique_ptr<statement_t>{std::move($1)}; }
| drop_statement { $$ = std::unique_ptr<statement_t>{std::move($1)}; }
;

create_statement:
  CREATE DATABASE opt_if_not_exists IDENTIFIER {
      $$ = std::unique_ptr<create_statement_t>{new create_statement_t(create_type_t::create_database, $4)};
      $$->if_not_exists = $3;
  }
| CREATE TABLE opt_if_not_exists composite_name "(" field_def_commalist ")" {
      $$ = std::unique_ptr<create_statement_t>{new create_statement_t(create_type_t::create_table, $4.second)};
      $$->if_not_exists = $3;
      $$->database = std::move($4.first);
      $$->fields = std::move(*$6);
  }
;

drop_statement:
  DROP TABLE composite_name {
      $$ = std::unique_ptr<drop_statement_t>{new drop_statement_t(drop_type_t::drop_table, $3.second)};
      $$->database = std::move($3.first);
  }
| DROP DATABASE IDENTIFIER {
      $$ = std::make_unique<drop_statement_t>(drop_type_t::drop_database, $3);
  }
;

field_def_commalist:
  field_def {
      $$ = std::unique_ptr<field_def_list_t>{new field_def_list_t()};
      $$->push_back(std::move($1)); }
| field_def_commalist "," field_def {
      $1->push_back(std::move($3));
      $$ = std::move($1);
  }
;

field_def:
  IDENTIFIER field_type opt_array {
      $$ = std::unique_ptr<field_definition_t>{new field_definition_t($1, $2->type, $3)};
  }
| IDENTIFIER field_type opt_array ACTIVE {
      $$ = std::unique_ptr<field_definition_t>{new field_definition_t($1, $2->type, $3)};
      $$->active = true;
  }
| IDENTIFIER REFERENCES composite_name  {
      $$ = std::unique_ptr<field_definition_t>{new field_definition_t($1, data_type_t::e_references, 1)};
      $$->table_type_database = std::move($3.first);
      $$->table_type_name = std::move($3.second);
  }
| REFERENCES composite_name {
      $$ = std::unique_ptr<field_definition_t>{new field_definition_t("", data_type_t::e_references, 1)};
      $$->table_type_database = std::move($2.first);
      $$->table_type_name = std::move($2.second);
  }
;

opt_array:
  "[" "]" { $$ = 0; }
| "[" NUMBER "]" { $$ = $2; }
| { $$ = 1; }
;

field_type:
  BOOL { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::e_bool)}; }
| INT8 { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::e_int8)}; }
| UINT8 { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::e_uint8)}; }
| INT16 { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::e_int16)}; }
| UINT16 { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::e_uint16)}; }
| INT32 { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::e_int32)}; }
| UINT32 { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::e_uint32)}; }
| INT64 { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::e_int64)}; }
| UINT64 { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::e_uint64)}; }
| FLOAT { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::e_float)}; }
| DOUBLE { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::e_double)}; }
| STRING { $$ = std::unique_ptr<field_type_t>{new field_type_t(data_type_t::e_string)}; }
;

composite_name:
  IDENTIFIER { $$ = std::make_pair("", $1); }
| IDENTIFIER "." IDENTIFIER { $$ = make_pair($1, $3); }
;

%%

void yy::parser::error (const location_type& l, const std::string& m) {
    std::cerr << l << ": " << m << '\n';
}
