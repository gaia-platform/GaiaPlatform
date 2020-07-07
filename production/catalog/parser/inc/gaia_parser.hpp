/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "gaia_catalog.hpp"
#include "yy_parser.hpp"
#include <string>
#include <vector>
#include <memory>

namespace gaia {
namespace catalog {
namespace ddl {

class parser_t {
  public:
    parser_t() : trace_parsing(false), trace_scanning(false){};

    // Use smart pointers to store the statements because we need the polymorphic behaviour.
    std::vector<unique_ptr<gaia::catalog::ddl::statement_t>> statements;

    int parse(const std::string &f) {
        file = f;
        location.initialize(&file);
        scan_begin();
        yy::parser parse(*this);
        parse.set_debug_level(trace_parsing);
        int res = parse();
        scan_end();
        return res;
    };

    yy::location location;
    bool trace_parsing;
    bool trace_scanning;

  private:
    std::string file;

    void scan_begin();
    void scan_end();
};

} // namespace ddl
} // namespace catalog
} // namespace gaia
