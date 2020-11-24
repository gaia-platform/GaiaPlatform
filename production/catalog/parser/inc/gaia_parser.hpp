/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "gaia/db/catalog.hpp"
#include "yy_parser.hpp"

namespace gaia
{
namespace catalog
{
namespace ddl
{

class parser_t
{
public:
    parser_t()
        : trace_parsing(false), trace_scanning(false){};

    // Use smart pointers to store the statements because we need the polymorphic behaviour.
    std::vector<std::unique_ptr<gaia::catalog::ddl::statement_t>> statements;

    int parse(const std::string& filename)
    {
        file = filename;
        location.initialize(&file);
        scan_begin();
        yy::parser parse(*this);
        parse.set_debug_level(trace_parsing);
        int res = parse();
        scan_end();
        return res;
    };

    int parse_line(const std::string& line)
    {
        scan_string_begin(line);
        yy::parser parse(*this);
        parse.set_debug_level(trace_parsing);
        int res = parse();
        scan_string_end();
        return res;
    }

    yy::location location;
    bool trace_parsing;
    bool trace_scanning;

private:
    std::string file;

    void scan_begin();
    void scan_end();

    void scan_string_begin(const std::string& line);
    void scan_string_end();
};

} // namespace ddl
} // namespace catalog
} // namespace gaia
