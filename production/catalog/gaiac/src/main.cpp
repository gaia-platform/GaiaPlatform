/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "driver.hpp"
#include "gaia_catalog.hpp"
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
    driver drv;
    gaia::db::gaia_mem_base::init(true);
    gaia::catalog::initialize_catalog(true);
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == std::string("-p")) {
            drv.trace_parsing = true;
        } else if (argv[i] == std::string("-s")) {
            drv.trace_scanning = true;
        } else if (!drv.parse(argv[i])) {
            execute(drv.statements);
        } else {
            res = EXIT_FAILURE;
        }
    }
    return res;
}
