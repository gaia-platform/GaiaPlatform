/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "driver.hpp"
#include "gaia_catalog.hpp"
#include <iostream>
#include <vector>

using namespace std;
using namespace gaia::catalog::ddl;

void execute(vector<Statement *> &statements) {
    for (Statement *stmt : statements) {
        if (!stmt->isType(StatementType::kStmtCreate)) {
            continue;
        }
        CreateStatement *createStmt = reinterpret_cast<CreateStatement *>(stmt);
        if (createStmt->type == CreateType::kCreateType) {
            gaia::catalog::create_type(createStmt->typeName,
                                       createStmt->fields);
        } else if (createStmt->type == CreateType::kCreateTableOf) {
            gaia::catalog::create_table_of(createStmt->tableName,
                                           createStmt->typeName);
        } else if (createStmt->type == CreateType::kCreateTable) {
            gaia::catalog::create_table(createStmt->tableName,
                                        createStmt->fields);
        }
    }
}

int main(int argc, char *argv[]) {
    int res = 0;
    driver drv;
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
