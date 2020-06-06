/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#ifndef __DRIVER_HPP
#define __DRIVER_HPP
#include "gaia_catalog.hpp"
#include "parser.hpp"
#include <map>
#include <string>
#include <vector>

#define YY_DECL yy::parser::symbol_type yylex(driver &drv)
YY_DECL;

class driver {
  public:
    driver() : trace_parsing(false), trace_scanning(false){};

    std::vector<gaia::catalog::ddl::statement_t *> statements;

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

    std::string file;
    bool trace_parsing;

    void scan_begin();
    void scan_end();
    bool trace_scanning;
    yy::location location;
};
#endif // !DRIVER_HPP
