#ifndef __DRIVER_HPP
#define __DRIVER_HPP
#include "ddl.hpp"
#include "parser.hpp"
#include <map>
#include <string>

#define YY_DECL yy::parser::symbol_type yylex(driver &drv)
YY_DECL;

// Conducting the whole scanning and parsing of DDLs
class driver {
  public:
    driver() : trace_parsing(false), trace_scanning(false) {
    };

    std::vector<gaia::catalog::ddl::Statement*> statements;

    // Run the parser on file F.  Return 0 on success.
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
    // The name of the file being parsed.
    std::string file;
    // Whether to generate parser debug traces.
    bool trace_parsing;

    // Handling the scanner.
    void scan_begin();
    void scan_end();
    // Whether to generate scanner debug traces.
    bool trace_scanning;
    // The token's location used by the scanner.
    yy::location location;
};
#endif // !DRIVER_HPP
