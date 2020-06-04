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
        class CompositeName;
        class Statement;
        class CreateStatement;
        class FieldDefinition;
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
    #include "ddl.hpp"
}

%define api.token.prefix {TOK_}
%token BOOLEAN BYTE UBYTE SHORT USHORT INT UINT LONG ULONG FLOAT DOUBLE STRING
%token CREATE TABLE TYPE SCHEMA AS
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

%type <CompositeName*> composite_name
%type <Statement*> statement
%type <CreateStatement*> create_statement
%type <DataType> field_type
%type <int> opt_array
%type <FieldDefinition*> field_def
%type <std::vector<FieldDefinition*>*> field_def_commalist
%type <std::vector<Statement*>*> statement_list

%printer { yyo << static_cast<unsigned int>($$); } <DataType>;
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
    CREATE TABLE composite_name "(" field_def_commalist ")" {
        $$ = new CreateStatement(CreateType::kCreateTable);
        $$->tableSchema = std::move($3->schema);
        $$->tableName = std::move($3->name);
        $$->fields = $5;
    };
    | CREATE TYPE composite_name "(" field_def_commalist ")" {
        $$ = new CreateStatement(CreateType::kCreateType);
        $$->typeSchema = std::move($3->schema);
        $$->typeName = std::move($3->name);
        $$->fields = $5;
    };
    | CREATE TABLE composite_name AS composite_name {
        $$ = new CreateStatement(CreateType::kCreateTableAs);
        $$->tableSchema = std::move($3->schema);
        $$->tableName = std::move($3->schema);
        $$->typeSchema = std::move($5->schema);
        $$->typeName = std::move($5->schema);
    };

field_def_commalist:
    field_def { $$ = new std::vector<FieldDefinition*>(); $$->push_back($1); }
    | field_def_commalist "," field_def { $1->push_back($3); $$ = $1; };

field_def:
    IDENTIFIER field_type opt_array { $$ = new FieldDefinition($1, $2, $3); };

opt_array:
    "[" "]" { $$ = 0; }
    | "[" NUMBER "]" { $$ = $2; }
    | { $$ = 1; } ;

field_type:
    BOOLEAN { $$ = DataType::BOOLEAN; }
    | BYTE { $$ = DataType::BYTE; }
    | UBYTE { $$ = DataType::UBYTE; }
    | SHORT { $$ = DataType::SHORT; }
    | USHORT { $$ = DataType::USHORT; }
    | INT { $$ = DataType::INT; }
    | UINT { $$ = DataType::UINT; }
    | LONG { $$ = DataType::LONG; }
    | ULONG { $$ = DataType::ULONG; }
    | FLOAT { $$ = DataType::FLOAT; }
    | DOUBLE { $$ = DataType::DOUBLE; }
    | STRING { $$ = DataType::STRING; };

composite_name:
    IDENTIFIER { $$ = new CompositeName($1); }
    | IDENTIFIER "." IDENTIFIER { $$ = new CompositeName($1, $3); };

%%
void
yy::parser::error (const location_type& l, const std::string& m)
{
  std::cerr << l << ": " << m << '\n';
}
