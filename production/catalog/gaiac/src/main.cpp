/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "catalog_manager.hpp"
#include "gaia_parser.hpp"
#include "gaia_system.hpp"
#include "gaia_db.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace gaia::catalog;
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

void load_bootstrap_catalog() {
    {
        field_def_list_t fields;
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"name", data_type_t::STRING, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"is_log", data_type_t::BOOL, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"trim_action", data_type_t::UINT8, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"max_rows", data_type_t::UINT64, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"max_size", data_type_t::UINT64, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"max_seconds", data_type_t::UINT64, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"binary_schema", data_type_t::STRING, 1}));
        catalog_manager_t::get().create_table("gaia_table", fields);
    }

    {
        field_def_list_t fields;
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"name", data_type_t::STRING, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"table_id", data_type_t::UINT64, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"fields", data_type_t::STRING, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"index_type", data_type_t::INT8, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"unique", data_type_t::BOOL, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"values_", data_type_t::REFERENCES, 1, "gaia_table"}));
        catalog_manager_t::get().create_table("gaia_value_index", fields);
    }

    {
        field_def_list_t fields;
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"name", data_type_t::STRING, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"table_id", data_type_t::UINT64, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"type", data_type_t::UINT8, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"type_id", data_type_t::UINT64, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"repeated_count", data_type_t::UINT8, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"position", data_type_t::UINT16, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"required", data_type_t::BOOL, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"deprecated", data_type_t::BOOL, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"active", data_type_t::BOOL, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"nullable", data_type_t::BOOL, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"has_default", data_type_t::BOOL, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"default_value", data_type_t::STRING, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"value_fields_", data_type_t::REFERENCES, 1, "gaia_value_index"}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"fields_", data_type_t::REFERENCES, 1, "gaia_table"}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"references_", data_type_t::REFERENCES, 1, "gaia_table"}));
        catalog_manager_t::get().create_table("gaia_field", fields);
    }

    {
        field_def_list_t fields;
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"name", data_type_t::STRING, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"active_on_startup", data_type_t::BOOL, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"table_ids", data_type_t::STRING, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"source_location", data_type_t::STRING, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"serial_stream", data_type_t::STRING, 1}));
        catalog_manager_t::get().create_table("gaia_ruleset", fields);
    }

        {
        field_def_list_t fields;
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"name", data_type_t::STRING, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"ruleset_id", data_type_t::UINT64, 1}));
        fields.push_back(unique_ptr<field_definition_t>(new field_definition_t{"rules_", data_type_t::REFERENCES, 1, "gaia_ruleset"}));
        catalog_manager_t::get().create_table("gaia_rule", fields);
    }
}

// From the database name and catalog contents, generate the flatbuffer andGaia EDC header files.
void generate_headers(string db_name) {
    // Generate the flatbuffer schema file.
    ofstream fbs(db_name + ".fbs");
    fbs << "namespace gaia." << db_name << ";" << endl << endl;
    fbs << gaia::catalog::generate_fbs() << endl;
    fbs.close();

    // Run the flatbuffer compiler, flatc, on this schema.
    string flatc_cmd = "flatc --cpp --gen-object-api ";
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
}

int main(int argc, char *argv[]) {
    int res = 0;
    parser_t parser;
    bool gen_catalog = true;
    gaia::db::begin_session();
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == string("-p")) {
            parser.trace_parsing = true;
        } else if (argv[i] == string("-s")) {
            parser.trace_scanning = true;
        } else if (argv[i] == string("-i")) {
            start_repl(parser);
            gen_catalog = false;
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

                generate_headers(db_name);

            } else {
                res = EXIT_FAILURE;
            }
            gen_catalog = false;
        }
    }
    if (gen_catalog) {
        load_bootstrap_catalog();
        generate_headers("catalog");
    }
    gaia::db::end_session();
    return res;
}
