%{ /* -*- C++ -*- */
/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
/*-------------------------------------------------------------------------
 *
 * lex.ll
 *   lexer for Gaia catalog DDLs
 *
 *-------------------------------------------------------------------------
 */
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <string>
#include "gaia_parser.hpp"
#include "yy_parser.hpp"
%}

%option noyywrap nounput noinput batch debug

%{
  yy::parser::symbol_type
  make_NUMBER (const std::string &s, const yy::parser::location_type& loc);
%}

id    [a-zA-Z][a-zA-Z_0-9]*
int   [0-9]+
blank [ \t\r]


%{
  #define YY_USER_ACTION loc.columns (yyleng);
  #define YY_DECL yy::parser::symbol_type yylex(gaia::catalog::ddl::parser_t &gaia_parser)
%}

%%

%{
  yy::location& loc = gaia_parser.location;
  loc.step ();
%}

{blank}+   loc.step ();
\n+        loc.lines (yyleng); loc.step ();

"CREATE"  return yy::parser::make_CREATE     (loc);
"TABLE"   return yy::parser::make_TABLE      (loc);
"BOOL"    return yy::parser::make_BOOL       (loc);
"INT8"    return yy::parser::make_INT8       (loc);
"UINT8"   return yy::parser::make_UINT8      (loc);
"INT16"   return yy::parser::make_INT16      (loc);
"UINT16"  return yy::parser::make_UINT16     (loc);
"INT32"   return yy::parser::make_INT32      (loc);
"UINT32"  return yy::parser::make_UINT32     (loc);
"INT64"   return yy::parser::make_INT64      (loc);
"UINT64"  return yy::parser::make_UINT64     (loc);
"FLOAT32" return yy::parser::make_FLOAT32    (loc);
"FLOAT64" return yy::parser::make_FLOAT64    (loc);
"STRING"  return yy::parser::make_STRING     (loc);
"("       return yy::parser::make_LPAREN     (loc);
")"       return yy::parser::make_RPAREN     (loc);
"["       return yy::parser::make_LBRACKET   (loc);
"]"       return yy::parser::make_RBRACKET   (loc);
","       return yy::parser::make_COMMA      (loc);
";"       return yy::parser::make_SEMICOLON  (loc);
{id}      return yy::parser::make_IDENTIFIER (yytext, loc);
{int}     return make_NUMBER                 (yytext, loc);

<<EOF>>    return yy::parser::make_END (loc);

%%

yy::parser::symbol_type
make_NUMBER (const std::string &s, const yy::parser::location_type& loc) {
    errno = 0;
    long n = strtol (s.c_str(), NULL, 10);
    if (! (INT_MIN <= n && n <= INT_MAX && errno != ERANGE)) {
        throw yy::parser::syntax_error (loc, "integer is out of range: " + s);
    }
    return yy::parser::make_NUMBER ((int) n, loc);
}

void gaia::catalog::ddl::parser_t::scan_begin () {
    yy_flex_debug = trace_scanning;
    if (file.empty () || file == "-") {
        yyin = stdin;
    } else if (!(yyin = fopen(file.c_str (), "r"))) {
        std::cerr << "cannot open " << file << ": " << strerror(errno) << '\n';
        exit (EXIT_FAILURE);
    }
}

void gaia::catalog::ddl::parser_t::scan_end () {
    fclose(yyin);
}

void gaia::catalog::ddl::parser_t::scan_string_begin (const string& input) {
    yy_flex_debug = trace_scanning;
    yy_scan_string(input.c_str());
}

void gaia::catalog::ddl::parser_t::scan_string_end () {
    yylex_destroy();
}
