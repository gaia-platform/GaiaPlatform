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
#include <sstream>
#include <gaia_spdlog/fmt/fmt.h>
#include "gaia_parser.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/scope_guard.hpp"
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
"REFERENCES"   return yy::parser::make_REFERENCES(loc);
"RELATIONSHIP" return yy::parser::make_RELATIONSHIP(loc);
"ACTIVE"       return yy::parser::make_ACTIVE(loc);
"USE"          return yy::parser::make_USE(loc);
"USING"        return yy::parser::make_USING(loc);
"WHERE"        return yy::parser::make_WHERE(loc);

"UNIQUE"     return yy::parser::make_UNIQUE(loc);
"RANGE"      return yy::parser::make_RANGE(loc);
"HASH"       return yy::parser::make_HASH(loc);
"INDEX"      return yy::parser::make_INDEX(loc);
"ON"         return yy::parser::make_ON(loc);

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
"="          return yy::parser::make_EQUAL(loc);

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

gaia::catalog::ddl::parsing_error::parsing_error(const std::string& message)
{
    m_message = message;
}

gaia::catalog::ddl::parsing_error::parsing_error(const yy::parser::location_type& parser_location, const std::string& message)
{
    std::stringstream message_stream;
    message_stream << "Parsing error at location " << parser_location << ": " << message;
    m_message = message_stream.str();
}

void gaia::catalog::ddl::parser_t::parse(const std::string& filename)
{
    m_file = filename;
    location.initialize(&m_file);
    scan_begin();
    const auto finish_scan = common::scope_guard::make_scope_guard([this]() { scan_end(); });

    yy::parser parse(*this);
    parse.set_debug_level(trace_parsing);
    int parsing_result = parse();

    // All parsing errors should be thrown as the 'parsing_error' exception.
    // Any violation below means there are unhandled parsing errors.
    ASSERT_POSTCONDITION(
        parsing_result == EXIT_SUCCESS,
        gaia_fmt::format("Failed to handle parsing errors in the DDL file '{}'.", filename).c_str());
}

void gaia::catalog::ddl::parser_t::parse_string(const std::string& str)
{
    scan_string_begin(str);
    const auto finish_scan_string = common::scope_guard::make_scope_guard([this]() { scan_string_end(); });

    yy::parser parse(*this);
    parse.set_debug_level(trace_parsing);
    int parsing_result = parse();

    // All parsing errors should be thrown as the 'parsing_error' exception.
    // Any violation below means there are unhandled parsing errors.
    ASSERT_POSTCONDITION(
        parsing_result == EXIT_SUCCESS,
        gaia_fmt::format("Failed to handle parsing errors in the line: '{}'.", str).c_str());
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
