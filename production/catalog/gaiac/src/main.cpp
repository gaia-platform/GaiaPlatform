/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "flatbuffers/idl.h"

#include "catalog_manager.hpp"
#include "gaia_parser.hpp"
#include "gaia_system.hpp"
#include "gaia_db.hpp"

using namespace std;
using namespace gaia::catalog;
using namespace gaia::catalog::ddl;

static const string c_error_prompt = "ERROR: ";
static const string c_warning_prompt = "WARNING: ";

enum class operate_mode_t {
    interactive,
    generation,
};

void execute(const string &dbname, vector<unique_ptr<statement_t>> &statements) {
    for (auto &stmt : statements) {
        if (stmt->is_type(statement_type_t::create)) {
            auto create_stmt = dynamic_cast<create_statement_t *>(stmt.get());
            if (create_stmt->type == create_type_t::create_table) {
                gaia::catalog::create_table(dbname, create_stmt->name, create_stmt->fields);
            }
        } else if (stmt->is_type(statement_type_t::drop)) {
            auto drop_stmt = dynamic_cast<drop_statement_t *>(stmt.get());
            if (drop_stmt->type == drop_type_t::drop_table) {
                gaia::catalog::drop_table(drop_stmt->name);
            }
        }
    }
}

void start_repl(parser_t &parser) {
    gaia::db::begin_session();
    gaia::catalog::initialize_catalog();

    const auto prompt = "gaiac> ";
    const auto exit_command = "exit";

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
                execute("", parser.statements);
                cout << gaia::catalog::generate_fbs() << flush;
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
    cout << c_warning_prompt << warn << endl
         << flush;
}
void LogCompilerError(const std::string &err) {
    cout << c_warning_prompt << err << endl
         << flush;
}
} // namespace flatbuffers

// From the database name and catalog contents, generate the FlatBuffers C++ header file(s).
void generate_fbs_headers(const string &db_name, const string &output_path) {
    flatbuffers::IDLOptions fbs_opts;
    fbs_opts.generate_object_based_api = true;
    fbs_opts.cpp_object_api_string_type = "gaia::direct_access::nullable_string_t";
    fbs_opts.cpp_object_api_string_flexible_constructor = true;

    flatbuffers::Parser fbs_parser(fbs_opts);

    string fbs_schema = "namespace gaia" +
                        (db_name.empty() ? "" : "." + db_name) +
                        ";\n" +
                        gaia::catalog::generate_fbs();
    if (!fbs_parser.Parse(fbs_schema.c_str())) {
        cout << c_error_prompt
             << "Fail to parse the catalog generated FlatBuffers schema. Error: "
             << fbs_parser.error_ << endl
             << flush;
    }

    if (!flatbuffers::GenerateCPP(fbs_parser, output_path, db_name)) {
        cout << c_error_prompt
             << "Unable to generate FlatBuffers C++ headers for " << db_name << endl
             << flush;
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

// Temporary start server (taken and modified from db_test_helpers)
// Use case is always to call start() followed by stop().
class db_server_t {
  public:
    void start(const char *db_server_path) {
        set_path(db_server_path);
        stop();

        // Launch SE server in background.
        string cmd = m_server_path + " &";
        cerr << cmd << endl;
        ::system(cmd.c_str());

        // Wait for server to initialize.
        cerr << "Waiting for server to initialize..." << endl;
        ::sleep(1);
        m_server_started = true;
    }

    void stop() {
        // Try to kill the SE server process.
        // REVIEW: we should be using a proper process library for this, so we can kill by PID.
        string cmd = "pkill -f -KILL ";
        cmd.append(m_server_path.c_str());
        cerr << cmd << endl;
        ::system(cmd.c_str());
    }

    bool server_started() {
        return m_server_started;
    }

  private:
    void set_path(const char *db_server_path) {
        if (!db_server_path) {
            m_server_path = gaia::db::SE_SERVER_NAME;
        } else {
            m_server_path = db_server_path;
            terminate_path(m_server_path);
            m_server_path.append(gaia::db::SE_SERVER_NAME);
        }
    }

    string m_server_path;
    bool m_server_started = false;
};

string usage() {
    std::stringstream ss;
    ss << "Usage: gaiac [options] [ddl_file]"
          "  where: -s          Print parsing trace."
          "         -p          Print scanning trace."
          "         -d <dbname> Set the databse name."
          "         -i          Interactive prompt, as a REPL."
          "         -o <path>   Set the path to all generated files."
          "         -t          Start the SE server (for testing purposes)."
          "         -h          Print help information."
          "         <ddl_file>  Process the DDLs in the file."
          "                     In the absence of <dbname>, the ddl file basename will be used as the database name."
          "                     The database will be created automatically.";
    return ss.str();
}

int main(int argc, char *argv[]) {
    int res = EXIT_SUCCESS;
    db_server_t server;
    string output_path;
    string db_name;
    string ddl_filename;
    operate_mode_t mode = operate_mode_t::generation;
    parser_t parser;

    for (int i = 1; i < argc; ++i) {
        if (argv[i] == string("-p")) {
            parser.trace_parsing = true;
        } else if (argv[i] == string("-s")) {
            parser.trace_scanning = true;
        } else if (argv[i] == string("-i")) {
            mode = operate_mode_t::interactive;
        } else if (argv[i] == string("-t")) {
            // Note the order dependency.
            // Require a path right after this
            if (++i > argc) {
                cout << c_error_prompt << "Missing path to db server." << endl
                     << flush;
                exit(EXIT_FAILURE);
            }
            const char *path_to_db_server = argv[i];
            server.start(path_to_db_server);
        } else if (argv[i] == string("-o")) {
            if (++i > argc) {
                cout << c_error_prompt << "Missing path to output directory." << endl
                     << flush;
                exit(EXIT_FAILURE);
            }
            output_path = argv[i];
            terminate_path(output_path);
        } else if (argv[i] == string("-d")) {
            if (++i > argc) {
                cout << c_error_prompt << "Missing database name." << endl
                     << flush;
                exit(EXIT_FAILURE);
            }
            db_name = argv[i];
        } else if (argv[i] == string("-h")) {
            cout << usage() << endl;
            exit(EXIT_SUCCESS);
        } else {
            ddl_filename = argv[i];
            int parsing_result = parser.parse(ddl_filename);
            if (parsing_result != EXIT_SUCCESS) {
                cout << c_error_prompt << "Fail to parse the ddl file '"
                     << ddl_filename << "''." << endl
                     << flush;
                exit(parsing_result);
            }
        }
    }

    if (mode == operate_mode_t::interactive) {
        start_repl(parser);
    } else if (mode == operate_mode_t::generation) {
        try {
            gaia::db::begin_session();
            gaia::catalog::initialize_catalog();

            if (db_name.empty() && !ddl_filename.empty()) {
                // Strip off the path and any suffix to get database name if database name is not specified.
                db_name = ddl_filename;
                if (db_name.find("/") != string::npos) {
                    db_name = db_name.substr(db_name.find_last_of("/") + 1);
                }
                if (db_name.find(".") != string::npos) {
                    db_name = db_name.substr(0, db_name.find_last_of("."));
                }
                create_database(db_name);
            }

            execute(db_name, parser.statements);
            generate_headers(db_name, output_path);
            gaia::db::end_session();
        } catch (gaia_exception &e) {
            cout << c_error_prompt << e.what() << endl;
            if (string(e.what()).find("connect failed") != string::npos) {
                cout << "May need to start the storage engine server." << endl;
            }
            res = EXIT_FAILURE;
        }
    }
    if (server.server_started()) {
        server.stop();
    }

    exit(res);
}
