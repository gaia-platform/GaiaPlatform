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
    #include "gaia_internal/catalog/catalog.hpp"

    namespace gaia
    {
    namespace catalog
    {
    namespace ddl
    {
    class parser_t;
    }
    }
    }

    using field_def_list_t = std::vector<std::unique_ptr<gaia::catalog::ddl::base_field_def_t>>;
    using statement_list_t = std::vector<std::unique_ptr<gaia::catalog::ddl::statement_t>>;
    using data_type_t = gaia::common::data_type_t;
    using cardinality_t = gaia::catalog::relationship_cardinality_t;
    using index_type_t = gaia::catalog::index_type_t;
    using composite_name_t = std::pair<std::string, std::string>;
    using field_list_t = std::vector<std::string>;
    using constraint_t = gaia::catalog::ddl::constraint_t;
    using constraint_list_t = gaia::catalog::ddl::constraint_list_t;
}

// The parsing context.
%param { gaia::catalog::ddl::parser_t& gaia_parser }

%locations

%define parse.trace
%define parse.error verbose

%code {
    #include "gaia_parser.hpp"

    using namespace gaia::catalog::ddl;
    yy::parser::symbol_type yylex(parser_t &);
}

%define api.token.prefix {TOK_}

// Type tokens
%token BOOL INT8 UINT8 INT16 UINT16 INT32 UINT32 INT64 UINT64 FLOAT DOUBLE STRING

// Word tokens
%token CREATE DROP DATABASE TABLE IF NOT EXISTS ACTIVE RELATIONSHIP USE USING
%token UNIQUE RANGE HASH INDEX ON

// Symbols
%token LPAREN "("
%token RPAREN ")"
%token LBRACKET "["
%token RBRACKET "]"
%token COMMA ","
%token DOT "."
%token SEMICOLON ";"

%token RARROW "->"

%token <std::string> IDENTIFIER "identifier"
%token <int> NUMBER "number"

%token END 0

%type <std::unique_ptr<gaia::catalog::ddl::statement_t>> statement
%type <std::unique_ptr<gaia::catalog::ddl::create_statement_t>> create_statement
%type <std::unique_ptr<gaia::catalog::ddl::create_database_t>> create_database
%type <std::unique_ptr<gaia::catalog::ddl::create_table_t>> create_table
%type <std::unique_ptr<gaia::catalog::ddl::create_relationship_t>> create_relationship
%type <std::unique_ptr<gaia::catalog::ddl::create_index_t>> create_index
%type <std::unique_ptr<gaia::catalog::ddl::drop_statement_t>> drop_statement
%type <std::unique_ptr<gaia::catalog::ddl::use_statement_t>> use_statement

%type <int> opt_array
%type <bool> opt_if_exists
%type <bool> opt_if_not_exists
%type <data_type_t> scalar_type
%type <std::unique_ptr<gaia::catalog::ddl::base_field_def_t>> field_def
%type <std::unique_ptr<gaia::catalog::ddl::data_field_def_t>> data_field_def
%type <std::unique_ptr<field_def_list_t>> field_def_commalist
%type <std::unique_ptr<statement_list_t>> statement_list
%type <composite_name_t> composite_name
%type <gaia::catalog::ddl::link_def_t> link_def
%type <bool> opt_unique
%type <index_type_t> opt_index_type
%type <field_list_t> field_commalist
%type <std::unique_ptr<constraint_t>> constraint_def
%type <constraint_list_t> constraint_list
%type <std::optional<constraint_list_t>> opt_constraint_list
%type <gaia::catalog::ddl::table_field_map_t> table_field_map

%printer { yyo << "statement"; } statement
%printer { yyo << "create_statement:" << $$->name; } create_statement
%printer { yyo << "create_database:" << $$->name; } create_database
%printer { yyo << "create_table:" << $$->name; } create_table
%printer { yyo << "create_relatinship:" << $$->name; } create_relationship
%printer { yyo << "create_index:" << $$->name; } create_index
%printer { yyo << "drop_statement:" << $$->name; } drop_statement
%printer { yyo << "use_statement:" << $$->name; } use_statement
%printer { yyo << "field_def:" << $$->name; } field_def
%printer { yyo << "data_field_def:" << $$->name; } data_field_def
%printer { yyo << "link_def:" << $$.name; } link_def
%printer { yyo << "field_def_commalist[" << ($$ ? $$->size() : 0) << "]"; } field_def_commalist
%printer { yyo << "statement_list[" << $$->size() << "]"; } statement_list
%printer { yyo << "composite_name: " << $$.first << "." << $$.second; } composite_name
%printer { yyo << "scalar_type: " << static_cast<uint8_t>($$); } scalar_type
%printer { yyo << "index_type: " << static_cast<uint8_t>($$); } opt_index_type
%printer { yyo << "field_commalist[" << $$.size() << "]"; } field_commalist
%printer { yyo << "constraint_def:" << static_cast<uint8_t>($$->type); } constraint_def
%printer { yyo << "constraint_list[" << $$.size() << "]"; } constraint_list
%printer { yyo << "opt_constraint_list"; } opt_constraint_list
%printer { yyo << "table_field_map:" << $$.first.table << "," << $$.second.table; } table_field_map
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

opt_if_exists: IF EXISTS { $$ = true; } | { $$ = false; };

opt_if_not_exists: IF NOT EXISTS { $$ = true; } | { $$ = false; };

statement:
  create_statement { $$ = std::unique_ptr<statement_t>{std::move($1)}; }
| drop_statement { $$ = std::unique_ptr<statement_t>{std::move($1)}; }
| use_statement { $$ = std::unique_ptr<statement_t>{std::move($1)}; }
;

create_statement:
  create_database { $$ = std::unique_ptr<create_statement_t>{std::move($1)}; }
| create_table { $$ = std::unique_ptr<create_statement_t>{std::move($1)}; }
| create_relationship { $$ = std::unique_ptr<create_statement_t>{std::move($1)}; }
| create_index { $$ = std::unique_ptr<create_statement_t>{std::move($1)}; }
;

create_database:
  CREATE DATABASE opt_if_not_exists IDENTIFIER {
      $$ = std::make_unique<create_database_t>($4);
      $$->if_not_exists = $3;
  }
;

create_table:
  CREATE TABLE opt_if_not_exists composite_name "(" field_def_commalist ")" {
      $$ = std::make_unique<create_table_t>($4.second);
      $$->if_not_exists = $3;
      $$->database = std::move($4.first);
      if ($6)
      {
          $$->fields = std::move(*$6);
      }
  }
;

create_relationship:
  CREATE RELATIONSHIP opt_if_not_exists IDENTIFIER "(" link_def "," link_def ")" {
      $$ = std::make_unique<create_relationship_t>($4);
      $$->relationship = std::make_pair($6, $8);
      $$->if_not_exists = $3;
  }
| CREATE RELATIONSHIP opt_if_not_exists IDENTIFIER "(" link_def "," link_def "," USING table_field_map")" {
      $$ = std::make_unique<create_relationship_t>($4);
      $$->relationship = std::make_pair($6, $8);
      $$->if_not_exists = $3;
      $$->field_map = $11;
  }
;

create_index:
  CREATE opt_unique opt_index_type INDEX opt_if_not_exists IDENTIFIER ON composite_name  "(" field_commalist ")" {
      $$ = std::make_unique<create_index_t>($6);
      $$->unique_index = $2;
      $$->index_type = $3;
      $$->if_not_exists = $5;
      $$->database = $8.first;
      $$->index_table = $8.second;
      $$->index_fields = std::move($10);
  }
;

drop_statement:
  DROP TABLE opt_if_exists composite_name {
      $$ = std::make_unique<drop_statement_t>(drop_type_t::drop_table, $4.second);
      $$->database = std::move($4.first);
      $$->if_exists = $3;
  }
| DROP DATABASE opt_if_exists IDENTIFIER {
      $$ = std::make_unique<drop_statement_t>(drop_type_t::drop_database, $4);
      $$->if_exists = $3;
  }
;

use_statement:
  USE IDENTIFIER {
      $$ = std::make_unique<use_statement_t>($2);
  }
;

field_def_commalist:
  field_def {
      $$ = std::make_unique<field_def_list_t>();
      $$->push_back(std::move($1)); }
| field_def_commalist "," field_def {
      $1->push_back(std::move($3));
      $$ = std::move($1);
  }
| {
      $$ = std::unique_ptr<field_def_list_t>{};
  }
;

field_def:
  data_field_def { $$ = std::unique_ptr<base_field_def_t>{std::move($1)}; }
;

data_field_def:
  IDENTIFIER scalar_type opt_array opt_constraint_list {
      $$ = std::make_unique<data_field_def_t>($1, $2, $3, $4);
  }
| IDENTIFIER STRING opt_constraint_list {
      $$ = std::make_unique<data_field_def_t>($1, data_type_t::e_string, 1, $3);
  }
;

constraint_list:
  constraint_def {
      $$ = constraint_list_t();
      $$.push_back(std::move($1));
  }
| constraint_list constraint_def {
      $1.push_back(std::move($2));
      $$ = std::move($1);
  }
;

constraint_def:
  ACTIVE {
      $$ = std::make_unique<active_constraint_t>();
  }
| UNIQUE {
      $$ = std::make_unique<unique_constraint_t>();
  }
;

opt_constraint_list:
  constraint_list {
      $$ = std::make_optional(std::move($1));
  }
| {
      $$ = std::nullopt;
  }
;

link_def:
  IDENTIFIER "." IDENTIFIER "->" composite_name opt_array {
      $$.from_table = $1;
      $$.name = $3;
      $$.to_database = $5.first;
      $$.to_table = $5.second;
      $$.cardinality = $6 ? cardinality_t::one : cardinality_t::many;
  }
| IDENTIFIER "." IDENTIFIER "." IDENTIFIER "->" composite_name opt_array {
      $$.from_database = $1;
      $$.from_table = $3;
      $$.name = $5;
      $$.to_database = $7.first;
      $$.to_table = $7.second;
      $$.cardinality = $8 ? cardinality_t::one : cardinality_t::many;
  }
;

table_field_map:
  composite_name "(" field_commalist ")" "," composite_name "(" field_commalist ")" {
      $$ = std::make_pair<table_field_list_t, table_field_list_t>(
          {$1.first, $1.second, std::move($3)},
          {$6.first, $6.second, std::move($8)});
  }
;

opt_array:
  "[" "]" { $$ = 0; }
| { $$ = 1; }
;

scalar_type:
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
;

composite_name:
  IDENTIFIER { $$ = std::make_pair("", $1); }
| IDENTIFIER "." IDENTIFIER { $$ = std::make_pair($1, $3); }
;

opt_unique: UNIQUE { $$ = true; } | { $$ = false; };

opt_index_type:
  RANGE { $$ = index_type_t::range; }
| HASH { $$ = index_type_t::hash; }
| { $$ = index_type_t::range; }
;

field_commalist:
  IDENTIFIER {
      $$ = { $1 };
  }
| field_commalist "," IDENTIFIER {
      $1.emplace_back($3);
  }
;

%%

void yy::parser::error(const location_type& l, const std::string& m)
{
    throw parsing_error(l, m);
}
