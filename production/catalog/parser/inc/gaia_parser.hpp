/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "gaia_internal/catalog/catalog.hpp"

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
    explicit parsing_error(const std::string& message);

    explicit parsing_error(const yy::parser::location_type& parser_location, const std::string& message);
};

class parser_t
{
public:
    parser_t() = default;

    void parse(const std::string& filename);
    void parse_string(const std::string& str);

    // Use smart pointers to store the statements because we need the polymorphic behaviour.
    std::vector<std::unique_ptr<gaia::catalog::ddl::statement_t>> statements;

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
