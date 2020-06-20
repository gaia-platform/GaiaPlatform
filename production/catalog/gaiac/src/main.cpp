/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "gaia_catalog.hpp"
#include "gaia_parser.hpp"
#include "gaia_system.hpp"
#include <vector>

using namespace std;
using namespace gaia::catalog::ddl;

void execute(vector<statement_t *> &statements) {
    for (statement_t *stmt : statements) {
        if (!stmt->is_type(statment_type_t::CREATE)) {
            continue;
        }
        create_statement_t *createStmt = reinterpret_cast<create_statement_t *>(stmt);
        if (createStmt->type == create_type_t::CREATE_TABLE) {
            gaia::catalog::create_table(createStmt->tableName, *createStmt->fields);
        }
    }
}

int main(int argc, char *argv[]) {
    int res = 0;
    parser_t parser;
    gaia::db::gaia_mem_base::init(true);
    gaia::catalog::initialize_catalog(true);
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == std::string("-p")) {
            parser.trace_parsing = true;
        } else if (argv[i] == std::string("-s")) {
            parser.trace_scanning = true;
        } else if (!parser.parse(argv[i])) {
            execute(parser.statements);
        } else {
            res = EXIT_FAILURE;
        }
    }
    return res;
}
