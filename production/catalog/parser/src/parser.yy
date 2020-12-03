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
    namespace common {
    enum class data_type_t : uint8_t;
    }
    namespace catalog {
    namespace ddl {
        struct statement_t;
        struct create_statement_t;
        struct drop_statement_t;
        enum class field_type_t : uint8_t;
        struct base_field_def_t;
        struct data_field_def_t;
        struct ref_field_def_t;
        class parser_t;
    } // namespace ddl
    } // namespace catalog
    } // namespace gaia

    using field_def_list_t = std::vector<std::unique_ptr<gaia::catalog::ddl::base_field_def_t>>;
    using statement_list_t = std::vector<std::unique_ptr<gaia::catalog::ddl::statement_t>>;
    using data_type_t = gaia::common::data_type_t;
    using composite_name_t = std::pair<std::string, std::string>;
}

// The parsing context.
%param { gaia::catalog::ddl::parser_t& gaia_parser }

%locations

%define parse.trace
%define parse.error verbose

%code {
    #include "gaia/db/catalog.hpp"
    #include "gaia_parser.hpp"

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

%type <int> opt_array
%type <bool> opt_if_not_exists
%type <data_type_t> data_type
%type <std::unique_ptr<gaia::catalog::ddl::base_field_def_t>> field_def
%type <std::unique_ptr<gaia::catalog::ddl::data_field_def_t>> data_field_def
%type <std::unique_ptr<gaia::catalog::ddl::ref_field_def_t>> ref_field_def
%type <std::unique_ptr<field_def_list_t>> field_def_commalist
%type <std::unique_ptr<statement_list_t>> statement_list
%type <composite_name_t> composite_name

%printer { yyo << "statement"; } statement
%printer { yyo << "create_statement:" << $$->name; } create_statement
%printer { yyo << "drop_statement:" << $$->name; } drop_statement
%printer { yyo << "filed_def:" << $$->name; } field_def
%printer { yyo << "data_field_def:" << $$->name; } data_field_def
%printer { yyo << "ref_field_def:" << $$->name; } ref_field_def
%printer { yyo << "filed_def_commalist[" << $$->size() << "]"; } field_def_commalist
%printer { yyo << "statement_list[" << $$->size() << "]"; } statement_list
%printer { yyo << "composite_name: " << $$.first << "." << $$.second; } composite_name
%printer { yyo << "data_type: " << static_cast<uint8_t>($$); } data_type
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
      $$ = std::make_unique<statement_list_t>();
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
      $$ = std::make_unique<create_statement_t>(create_type_t::create_database, $4);
      $$->if_not_exists = $3;
  }
| CREATE TABLE opt_if_not_exists composite_name "(" field_def_commalist ")" {
      $$ = std::make_unique<create_statement_t>(create_type_t::create_table, $4.second);
      $$->if_not_exists = $3;
      $$->database = std::move($4.first);
      $$->fields = std::move(*$6);
  }
;

drop_statement:
  DROP TABLE composite_name {
      $$ = std::make_unique<drop_statement_t>(drop_type_t::drop_table, $3.second);
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
  data_field_def { $$ = std::unique_ptr<base_field_def_t>{std::move($1)}; }
| ref_field_def { $$ = std::unique_ptr<base_field_def_t>{std::move($1)}; }
;

data_field_def:
  IDENTIFIER data_type opt_array {
      $$ = std::make_unique<data_field_def_t>($1, $2, $3);
  }
| IDENTIFIER data_type opt_array ACTIVE {
      $$ = std::make_unique<data_field_def_t>($1, $2, $3);
      $$->active = true;
  }
;

ref_field_def:
 IDENTIFIER REFERENCES composite_name  {
      $$ = std::make_unique<ref_field_def_t>($1, $3);
  }
| REFERENCES composite_name {
      $$ = std::make_unique<ref_field_def_t>("", $2);
  }
;

opt_array:
  "[" "]" { $$ = 0; }
| "[" NUMBER "]" { $$ = $2; }
| { $$ = 1; }
;

data_type:
  BOOL { $$ = data_type_t::e_bool; }
| INT8 { $$ = data_type_t::e_int8; }
| UINT8 { $$ = data_type_t::e_uint8; }
| INT16 { $$ = data_type_t::e_int16; }
| UINT16 { $$ = data_type_t::e_uint16; }
| INT32 { $$ = data_type_t::e_int32; }
| UINT32 { $$ = data_type_t::e_uint32; }
| INT64 { $$ = data_type_t::e_int64; }
| UINT64 { $$ = data_type_t::e_uint64; }
| FLOAT { $$ = data_type_t::e_float; }
| DOUBLE { $$ = data_type_t::e_double; }
| STRING { $$ = data_type_t::e_string; }
;

composite_name:
  IDENTIFIER { $$ = std::make_pair("", $1); }
| IDENTIFIER "." IDENTIFIER { $$ = std::make_pair($1, $3); }
;

%%

void yy::parser::error (const location_type& l, const std::string& m) {
    std::cerr << l << ": " << m << '\n';
}
