/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include <unistd.h>
#include <stdlib.h> 
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "flatbuffers/idl.h"

#include "catalog_manager.hpp"
#include "gaia_catalog_internal.hpp"
#include "gaia_parser.hpp"
#include "gaia_system.hpp"
#include "gaia_db.hpp"
#include "db_test_base.hpp"

using namespace std;
using namespace gaia::catalog;
using namespace gaia::catalog::ddl;

static const string c_error_prompt = "ERROR: ";
static const string c_warning_prompt = "WARNING: ";

enum class operate_mode_t {
    interactive,
    generation,
    loading,
};

void start_repl(parser_t &parser, const string &dbname) {
    gaia::db::begin_session();

    const auto prompt = "gaiac> ";
    const auto exit_command = "exit";

    // NOTE: all REPL outputs including error messages go to standarad output.
    while (true) {
        string line;
        cout << prompt << flush;
        if (!getline(cin, line)) {
            break;
        }
        if (line == exit_command) {
            break;
        }
        int parsing_result = parser.parse_line(line);
        if (parsing_result == EXIT_SUCCESS) {
            try {
                execute(dbname, parser.statements);
                cout << gaia::catalog::generate_fbs(dbname) << flush;
            } catch (gaia_exception &e) {
                cout << c_error_prompt << e.what() << endl
                     << flush;
            }
        } else {
            cout << c_error_prompt << "Invalid input." << endl
                 << flush;
        }
    }

    gaia::db::end_session();
}

namespace flatbuffers {
void LogCompilerWarn(const std::string &warn) {
    cerr << c_warning_prompt << warn << endl;
}

void LogCompilerError(const std::string &err) {
    cerr << c_warning_prompt << err << endl;
}
} // namespace flatbuffers

// From the database name and catalog contents, generate the FlatBuffers C++ header file(s).
void generate_fbs_headers(const string &db_name, const string &output_path) {
    flatbuffers::IDLOptions fbs_opts;
    fbs_opts.generate_object_based_api = true;
    fbs_opts.cpp_object_api_string_type = "gaia::direct_access::nullable_string_t";
    fbs_opts.cpp_object_api_string_flexible_constructor = true;

    flatbuffers::Parser fbs_parser(fbs_opts);

    string fbs_schema = gaia::catalog::generate_fbs(db_name);
    if (!fbs_parser.Parse(fbs_schema.c_str())) {
        cerr << c_error_prompt
             << "Fail to parse the catalog generated FlatBuffers schema. Error: "
             << fbs_parser.error_ << endl;
    }

    if (!flatbuffers::GenerateCPP(fbs_parser, output_path, db_name)) {
        cerr << c_error_prompt
             << "Unable to generate FlatBuffers C++ headers for " << db_name << endl;
    };
}

// From the database name and catalog contents, generate the Extended Data Class definition(s).
void generate_edc_headers(const string &db_name, const string &output_path) {
    ofstream edc(output_path + "gaia" + (db_name.empty() ? "" : "_" + db_name) + ".h");
    edc << gaia::catalog::gaia_generate(db_name) << endl;
    edc.close();
}

void generate_headers(const string &db_name, const string &output_path) {
    generate_fbs_headers(db_name, output_path);
    generate_edc_headers(db_name, output_path);
}

// Add a trailing '/' if not provided.
void terminate_path(string &path) {
    if (path.back() != '/') {
        path.append("/");
    }
}

string usage() {
    std::stringstream ss;
    ss << "Usage: gaiac [options] [ddl_file]\n\n"
          "  -p          Print parsing trace.\n"
          "  -s          Print scanning trace.\n"
          "  -d <dbname> Set the databse name.\n"
          "  -i          Interactive prompt, as a REPL.\n"
          "  -g          Generate fbs and gaia headers.\n"
          "  -o <path>   Set the path to all generated files.\n"
          "  -t          Start the SE server (for testing purposes).\n"
          "  -h          Print help information.\n"
          "  <ddl_file>  Process the DDLs in the file.\n"
          "              In the absence of <dbname>, the ddl file basename will be used as the database name.\n"
          "              The database will be created automatically.\n";
    return ss.str();
}

int main(int argc, char *argv[]) {
    int res = EXIT_SUCCESS;
    db_server_t server;
    string output_path;
    string db_name;
    string ddl_filename;
    operate_mode_t mode = operate_mode_t::loading;
    parser_t parser;

    for (int i = 1; i < argc; ++i) {
        if (argv[i] == string("-p")) {
            parser.trace_parsing = true;
        } else if (argv[i] == string("-s")) {
            parser.trace_scanning = true;
        } else if (argv[i] == string("-i")) {
            mode = operate_mode_t::interactive;
        } else if (argv[i] == string("-g")) {
            mode = operate_mode_t::generation;
        } else if (argv[i] == string("-t")) {
            if (++i > argc) {
                cerr << c_error_prompt << "Missing path to db server." << endl;
                exit(EXIT_FAILURE);
            }
            // db_test_base_t::remove_persistent_store();
            const char *path_to_db_server = argv[i];
            server.start(path_to_db_server);
        } else if (argv[i] == string("-o")) {
            if (++i > argc) {
                cerr << c_error_prompt << "Missing path to output directory." << endl;
                exit(EXIT_FAILURE);
            }
            output_path = argv[i];
            terminate_path(output_path);
        } else if (argv[i] == string("-d")) {
            if (++i > argc) {
                cerr << c_error_prompt << "Missing database name." << endl;
                exit(EXIT_FAILURE);
            }
            db_name = argv[i];
        } else if (argv[i] == string("-h")) {
            cout << usage() << endl;
            exit(EXIT_SUCCESS);
        } else {
            ddl_filename = argv[i];
        }
    }

    if (mode == operate_mode_t::interactive) {
        start_repl(parser, db_name);
    } else {
        try {
            gaia::db::begin_session();

            if (!ddl_filename.empty()) {
                db_name = load_catalog(parser, ddl_filename, db_name);
            }

            if (mode == operate_mode_t::generation) {
                generate_headers(db_name, output_path);
            }
            gaia::db::end_session();
        } catch (gaia_exception &e) {
            cerr << c_error_prompt << e.what() << endl;
            if (string(e.what()).find("connect failed") != string::npos) {
                cerr << "May need to start the storage engine server." << endl;
            }
            res = EXIT_FAILURE;
        }
    }
    if (server.server_started()) {
        server.stop();
    }

    // Hardcode the path to persistent store for now & expose this option later.
    // db_test_base_t::remove_persistent_store();

    exit(res);
}
