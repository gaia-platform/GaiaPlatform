%{ /* -*- C++ -*- */
/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
/**
 * Gaia data definition language (DDL) Flex input file
 *
 * Coding style for this file:
 * - The braced C++ code follows "Gaia C++ Coding Guidelines".
 * - Align pattens and actions as two tabular columns in the rules section.
 */
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <string>
#include "gaia_parser.hpp"
#include "yy_parser.hpp"
%}

%option caseless noyywrap nounput noinput batch debug

%{
    yy::parser::symbol_type
    make_NUMBER (const std::string &s, const yy::parser::location_type& loc);
%}

/* The exclusive start condition for C style comments. */
%x xcomment

id    [a-zA-Z][a-zA-Z_0-9]*
int   [0-9]+
blank [ \t\r]

comment ("--".*)

%{
    #define YY_USER_ACTION loc.columns (yyleng);
    #define YY_DECL yy::parser::symbol_type yylex(gaia::catalog::ddl::parser_t &gaia_parser)
%}

%%

%{
    yy::location& loc = gaia_parser.location;
    loc.step ();
%}


"/*"              BEGIN(xcomment);
<xcomment>"*/"    BEGIN(INITIAL);
<xcomment>[^*\n]+ { /* ignore anything that's not a '*' */ }
<xcomment>"*"[^/] { /* ignore '*' not followed by a '/' */ }
<xcomment>\n      loc.lines(yyleng);


{blank}+     loc.step();
{comment}    { /* ignore */ }
\n+          loc.lines(yyleng); loc.step();


"CREATE"       return yy::parser::make_CREATE(loc);
"DROP"         return yy::parser::make_DROP(loc);
"DATABASE"     return yy::parser::make_DATABASE(loc);
"TABLE"        return yy::parser::make_TABLE(loc);
"IF"           return yy::parser::make_IF(loc);
"NOT"          return yy::parser::make_NOT(loc);
"EXISTS"       return yy::parser::make_EXISTS(loc);
"RELATIONSHIP" return yy::parser::make_RELATIONSHIP(loc);
"ACTIVE"       return yy::parser::make_ACTIVE(loc);
"USE"          return yy::parser::make_USE(loc);

"BOOL"       return yy::parser::make_BOOL(loc);
"INT8"       return yy::parser::make_INT8(loc);
"UINT8"      return yy::parser::make_UINT8(loc);
"INT16"      return yy::parser::make_INT16(loc);
"UINT16"     return yy::parser::make_UINT16(loc);
"INT32"      return yy::parser::make_INT32(loc);
"UINT32"     return yy::parser::make_UINT32(loc);
"INT64"      return yy::parser::make_INT64(loc);
"UINT64"     return yy::parser::make_UINT64(loc);
"FLOAT"      return yy::parser::make_FLOAT(loc);
"DOUBLE"     return yy::parser::make_DOUBLE(loc);
"STRING"     return yy::parser::make_STRING(loc);

"("          return yy::parser::make_LPAREN(loc);
")"          return yy::parser::make_RPAREN(loc);
"["          return yy::parser::make_LBRACKET(loc);
"]"          return yy::parser::make_RBRACKET(loc);
","          return yy::parser::make_COMMA(loc);
"."          return yy::parser::make_DOT(loc);
";"          return yy::parser::make_SEMICOLON(loc);

"->"         return yy::parser::make_RARROW(loc);

{id}         return yy::parser::make_IDENTIFIER(yytext, loc);
{int}        return make_NUMBER(yytext, loc);


<<EOF>>      return yy::parser::make_END(loc);

.            throw gaia::catalog::ddl::parsing_error(loc, "Invalid character '" + std::string(yytext) + "'");

%%

yy::parser::symbol_type
make_NUMBER(const std::string &s, const yy::parser::location_type& loc)
{
    errno = 0;
    long n = strtol(s.c_str(), NULL, 10);
    if (! (INT_MIN <= n && n <= INT_MAX && errno != ERANGE))
    {
        throw gaia::catalog::ddl::parsing_error(loc, "The integer " + s + " is out of range.");
    }
    return yy::parser::make_NUMBER ((int) n, loc);
}

void gaia::catalog::ddl::parser_t::scan_begin()
{
    yy_flex_debug = trace_scanning;
    if (m_file.empty () || m_file == "-")
    {
        yyin = stdin;
    }
    else if (!(yyin = fopen(m_file.c_str (), "r")))
    {
        std::stringstream err;
        err << "Cannot open " << m_file << ": " << strerror(errno);
        throw gaia::catalog::ddl::parsing_error(err.str());
    }
}

void gaia::catalog::ddl::parser_t::scan_end()
{
    fclose(yyin);
    yylex_destroy();
}

void gaia::catalog::ddl::parser_t::scan_string_begin(const std::string& input)
{
    yy_flex_debug = trace_scanning;
    yy_scan_string(input.c_str());
}

void gaia::catalog::ddl::parser_t::scan_string_end()
{
    yylex_destroy();
}
