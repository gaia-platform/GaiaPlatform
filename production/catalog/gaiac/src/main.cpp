/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "gaia_catalog.hpp"
#include "gaia_parser.hpp"
#include "gaia_system.hpp"
#include "gaia_db.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace gaia::catalog::ddl;

void execute(vector<unique_ptr<statement_t>> &statements) {
    for (auto &stmt : statements) {
        if (!stmt->is_type(statement_type_t::e_create)) {
            continue;
        }
        auto createStmt = dynamic_cast<create_statement_t *>(stmt.get());
        if (createStmt->type == create_type_t::e_create_table) {
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
        } else {
            if (!parser.parse(argv[i])) {
                execute(parser.statements);
                // Strip off the path and any suffix to get database name.
                string db_name = string(argv[i]);
                if (db_name.find("/") != string::npos) {
                    db_name = db_name.substr(db_name.find_last_of("/")+1);
                }
                if (db_name.find(".") != string::npos) {
                    db_name = db_name.substr(0, db_name.find_last_of("."));
                }

                // Generate the flatbuffer schema file.
                ofstream fbs(db_name + ".fbs");
                fbs << "namespace gaia." << db_name << ";" << endl << endl;
                fbs << gaia::catalog::generate_fbs() << endl;
                fbs.close();

                // Run the flatbuffer compiler, flatc, on this schema.
                string flatc_cmd = "flatc --cpp --gen-mutable --gen-setters --gen-object-api ";
                flatc_cmd += "--cpp-str-type gaia::direct_access::nullable_string_t --cpp-str-flex-ctor ";
                flatc_cmd += db_name + ".fbs";
                auto rc = system(flatc_cmd.c_str());

                // Generate the Extended Data Class definitions
                ofstream edc("gaia_" + db_name + ".h");
                edc << gaia::catalog::gaia_generate(db_name) << endl;
                edc.close();

                if (rc != 0) {
                    cout << "flatc failed to compile " + db_name + ".fbs" ", return code = " << rc << endl;
                }
            } else {
                res = EXIT_FAILURE;
            }
        }
    }
    gaia::db::end_session();
    return res;
}
