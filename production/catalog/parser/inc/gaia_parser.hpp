/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/common/assert.hpp"
#include "gaia_internal/common/scope_guard.hpp"

#include "yy_parser.hpp"

namespace gaia
{
namespace catalog
{
namespace ddl
{

/**
 * Thrown when hit DDL parsing error(s).
 */
class parsing_error : public gaia::common::gaia_exception
{
public:
    explicit parsing_error(const std::string& message)
    {
        m_message = message;
    }

    explicit parsing_error(const yy::parser::location_type& parser_location, const std::string& message)
    {
        std::stringstream message_stream;
        message_stream << "Parsing error at location " << parser_location << ": " << message;
        m_message = message_stream.str();
    }
};

class parser_t
{
public:
    parser_t() = default;

    // Use smart pointers to store the statements because we need the polymorphic behaviour.
    std::vector<std::unique_ptr<gaia::catalog::ddl::statement_t>> statements;

    void parse(const std::string& filename)
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
            "Failed to handle parsing errors in the DDL file '" + filename + "'.");
    };

    void parse_line(const std::string& line)
    {
        scan_string_begin(line);
        const auto finish_scan_string = common::scope_guard::make_scope_guard([this]() { scan_string_end(); });

        yy::parser parse(*this);
        parse.set_debug_level(trace_parsing);
        int parsing_result = parse();

        // All parsing errors should be thrown as the 'parsing_error' exception.
        // Any violation below means there are unhandled parsing errors.
        ASSERT_POSTCONDITION(
            parsing_result == EXIT_SUCCESS,
            "Failed to handle parsing errors in the line: '" + line + "'.");
    }

    yy::location location;
    bool trace_parsing{};
    bool trace_scanning{};

private:
    std::string m_file;

    void scan_begin();
    void scan_end();

    void scan_string_begin(const std::string& line);
    void scan_string_end();
};

} // namespace ddl
} // namespace catalog
} // namespace gaia
