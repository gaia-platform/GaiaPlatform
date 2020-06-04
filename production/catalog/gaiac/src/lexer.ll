%{ /* -*- C++ -*- */
/*-------------------------------------------------------------------------
 *
 * lex.l
 *   lexer for Gaia catalog DDLs
 *
 *-------------------------------------------------------------------------
 */
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <string>
#include "driver.hpp"
#include "parser.hpp"
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
%}

%%

%{
  yy::location& loc = drv.location;
  loc.step ();
%}

{blank}+   loc.step ();
\n+        loc.lines (yyleng); loc.step ();

"CREATE"  return yy::parser::make_CREATE     (loc);
"TABLE"   return yy::parser::make_TABLE      (loc);
"TYPE"    return yy::parser::make_TYPE       (loc);
"SCHEMA"  return yy::parser::make_SCHEMA     (loc);
"AS"      return yy::parser::make_AS         (loc);
"BOOLEAN" return yy::parser::make_BOOLEAN    (loc);
"BYTE"    return yy::parser::make_BYTE       (loc);
"UBYTE"   return yy::parser::make_UBYTE      (loc);
"SHORT"   return yy::parser::make_SHORT      (loc);
"USHORT"  return yy::parser::make_USHORT     (loc);
"INT"     return yy::parser::make_INT        (loc);
"UINT"    return yy::parser::make_UINT       (loc);
"LONG"    return yy::parser::make_LONG       (loc);
"ULONG"   return yy::parser::make_ULONG      (loc);
"FLOAT"   return yy::parser::make_FLOAT      (loc);
"DOUBLE"  return yy::parser::make_DOUBLE     (loc);
"STRING"  return yy::parser::make_STRING     (loc);
"("       return yy::parser::make_LPAREN     (loc);
")"       return yy::parser::make_RPAREN     (loc);
"["       return yy::parser::make_LBRACKET   (loc);
"]"       return yy::parser::make_RBRACKET   (loc);
","       return yy::parser::make_COMMA      (loc);
"."       return yy::parser::make_DOT        (loc);
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

void
driver::scan_begin ()
{
    yy_flex_debug = trace_scanning;
    if (file.empty () || file == "-") {
        yyin = stdin;
    } else if (!(yyin = fopen (file.c_str (), "r"))) {
        std::cerr << "cannot open " << file << ": " << strerror (errno) << '\n';
        exit (EXIT_FAILURE);
    }
}

void
driver::scan_end () {
    fclose (yyin);
}
