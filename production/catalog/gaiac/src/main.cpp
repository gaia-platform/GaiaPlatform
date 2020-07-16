/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "gaia_catalog.hpp"
#include "gaia_parser.hpp"
#include "gaia_db.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace gaia::catalog::ddl;

void execute(vector<unique_ptr<statement_t>> &statements) {
    for (auto &stmt : statements) {
        if (!stmt->is_type(statement_type_t::CREATE)) {
            continue;
        }
        auto createStmt = dynamic_cast<create_statement_t *>(stmt.get());
        if (createStmt->type == create_type_t::CREATE_TABLE) {
            gaia::catalog::create_table(createStmt->table_name, createStmt->fields);
        }
    }
}

void start_repl(parser_t &parser) {
    const auto prompt = "gaiac> ";

    while (true) {
        string line;
        cout << prompt << flush;
        if (!getline(cin, line)) {
            break;
        }
        if (line == "exit") {
            break;
        }
        int parsing_result = parser.parse_line(line);
        if (parsing_result == EXIT_SUCCESS) {
            try {
                execute(parser.statements);
                cout << gaia::catalog::generate_fbs() << flush;
            } catch (gaia_exception &e) {
                cout << e.what() << endl
                     << flush;
            }
        } else {
            cout << "Invalid input." << endl
                 << flush;
        }
    }
}

int main(int argc, char *argv[]) {
    int res = 0;
    parser_t parser;
    gaia::db::begin_session();
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == string("-p")) {
            parser.trace_parsing = true;
        } else if (argv[i] == string("-s")) {
            parser.trace_scanning = true;
        } else if (argv[i] == string("-i")) {
            start_repl(parser);
        } else if (!parser.parse(argv[i])) {
            execute(parser.statements);
            cout << gaia::catalog::generate_fbs() << endl;
        } else {
            res = EXIT_FAILURE;
        }
    }
    gaia::db::end_session();
    return res;
}
