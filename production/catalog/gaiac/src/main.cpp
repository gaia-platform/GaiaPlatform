/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cctype>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "flatbuffers/idl.h"

#include "gaia/common.hpp"
#include "gaia/db/db.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/ddl_execution.hpp"
#include "gaia_internal/common/gaia_version.hpp"
#include "gaia_internal/common/logger_internal.hpp"
#include "gaia_internal/common/scope_guard.hpp"
#include "gaia_internal/db/db_server_instance.hpp"
#include <gaia_internal/common/system_error.hpp>
#include <gaia_internal/db/gaia_db_internal.hpp>

#include "command.hpp"
#include "gaia_parser.hpp"

using namespace std;
using namespace gaia::catalog;
using namespace gaia::catalog::ddl;
using namespace gaia::common;
using namespace gaia::db;

namespace
{ // Use unnamed namespace to restrict external linkage.

const string c_error_prompt = "ERROR: ";
const string c_warning_prompt = "WARNING: ";

enum class operate_mode_t
{
    interactive,
    generation,
    loading,
};

void start_repl(parser_t& parser)
{
    initialize_catalog();

    const auto prompt = "gaiac> ";
    const auto exit_command = "exit";

    while (true)
    {
        string line;
        cout << prompt << flush;
        if (!getline(cin, line))
        {
            break;
        }
        if (line == exit_command)
        {
            break;
        }
        try
        {
            if (line.length() > 0 && line.at(0) == c_command_prefix)
            {
                if (handle_meta_command(line))
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
            parser.parse_line(line);
            execute(parser.statements);
        }
        catch (gaia::common::gaia_exception& e)
        {
            cerr << c_error_prompt << e.what() << endl;
        }
    }
}

// From the database name and catalog contents, generate the FlatBuffers C++ header file(s).
void generate_fbs_headers(const string& db_name, const string& output_path)
{
    flatbuffers::IDLOptions fbs_opts;
    fbs_opts.generate_object_based_api = true;
    fbs_opts.cpp_object_api_string_type = "gaia::direct_access::nullable_string_t";
    fbs_opts.cpp_includes = {"gaia/direct_access/nullable_string.hpp"};
    fbs_opts.cpp_object_api_string_flexible_constructor = true;

    flatbuffers::Parser fbs_parser(fbs_opts);

    string fbs_schema = gaia::catalog::generate_fbs(db_name);
    if (!fbs_parser.Parse(fbs_schema.c_str()))
    {
        cerr << c_error_prompt
             << "Failed to parse the catalog-generated FlatBuffers schema. Error: '"
             << fbs_parser.error_ << "'." << endl;
    }

    if (!flatbuffers::GenerateCPP(fbs_parser, output_path, db_name))
    {
        cerr << c_error_prompt
             << "Unable to generate FlatBuffers C++ headers for '" << db_name << "'." << endl;
    };
}

// From the database name and catalog contents, generate the Extended Data Class definition(s).
void generate_edc_headers(const string& db_name, const filesystem::path& output_path)
{
    filesystem::path header_path = output_path;
    header_path /= "gaia" + (db_name.empty() ? "" : "_" + db_name) + ".h";

    ofstream edc(header_path);
    try
    {
        edc << gaia::catalog::gaia_generate(db_name) << endl;
    }
    catch (gaia::common::gaia_exception& e)
    {
        cerr << "WARNING - gaia_generate failed: '" << e.what() << "'." << endl;
    }

    edc.close();
}

void generate_headers(const string& db_name, const filesystem::path& output_path)
{
    filesystem::path absolute_output_path = filesystem::absolute(output_path);
    absolute_output_path += filesystem::path::preferred_separator;
    absolute_output_path = absolute_output_path.lexically_normal();

    if (!filesystem::exists(absolute_output_path))
    {
        filesystem::create_directory(output_path);
    }
    else if (!filesystem::is_directory(absolute_output_path))
    {
        throw std::invalid_argument("Invalid output path: '" + output_path.string() + "'.");
    }

    cout << "Generating headers in: " << absolute_output_path << "." << endl;

    generate_fbs_headers(db_name, absolute_output_path);
    generate_edc_headers(db_name, absolute_output_path);
}

// Check if a database name is valid.
//
// Incorrect database names will result downstream failures in FlatBuffers and
// our own parser. We want to detect errors early on at command line parsing
// time to produce a more meaningful error message to the user. This function
// needs to keep in sync with fbs and our own ddl identifier rules.
bool valid_db_name(const string& db_name)
{
    if (db_name.empty())
    {
        return true;
    }
    if (!isalpha(db_name.front()))
    {
        return false;
    }
    for (char c : db_name.substr(1))
    {
        if (!(isalnum(c) || c == '_'))
        {
            return false;
        }
    }
    return true;
}

string usage()
{
    std::stringstream ss;
    ss << "Usage: gaiac [options] [ddl_file]\n\n"
          "  -d|--db-name <dbname>    Specify the database name.\n"
          "  -i|--interactive         Interactive prompt, as a REPL.\n"
          "  -g|--generate            Generate direct access API header files.\n"
          "  -o|--output <path>       Set the path to all generated files.\n"
#ifdef DEBUG
          "  -p|--parse-trace         Print parsing trace.\n"
          "  -s|--scan-trace          Print scanning trace.\n"
          "  -t|--db-server-path      Start the DB server (for testing purposes).\n"
          "  --destroy-db             Destroy the persistent store.\n"
#endif
          "  <ddl_file>               Process the DDLs in the file.\n"
          "                           In the absence of <dbname>, the ddl file basename will be used as the database name.\n"
          "                           The database will be created automatically.\n"
          "  -h|--help                Print help information.\n"
          "  -v|--version             Version information.\n";
    return ss.str();
}

string version()
{
    std::stringstream ss;
    ss << "Gaia Catalog Tool " << gaia_full_version() << "\nCopyright (c) Gaia Platform LLC\n";
    return ss.str();
}

} // namespace

namespace flatbuffers
{

// NOLINTNEXTLINE(readability-identifier-naming)
void LogCompilerWarn(const std::string& warn)
{
    cerr << c_warning_prompt << warn << endl;
}

// NOLINTNEXTLINE(readability-identifier-naming)
void LogCompilerError(const std::string& err)
{
    cerr << c_error_prompt << err << endl;
}

} // namespace flatbuffers

int main(int argc, char* argv[])
{
    gaia_log::initialize({});

    server_instance_t server;
    string output_path;
    vector<string> db_names;
    string ddl_filename;
    operate_mode_t mode = operate_mode_t::loading;
    parser_t parser;
    bool remove_persistent_store = false;
    const char* path_to_db_server = nullptr;

    // If no arguments are specified print the help.
    if (argc == 1)
    {
        cerr << usage() << endl;
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] == string("-p") || argv[i] == string("--parse-trace"))
        {
            parser.trace_parsing = true;
        }
        else if (argv[i] == string("-s") || argv[i] == string("--scan-trace"))
        {
            parser.trace_scanning = true;
        }
        else if (argv[i] == string("-i") || argv[i] == string("--interactive"))
        {
            mode = operate_mode_t::interactive;
        }
        else if (argv[i] == string("-g") || argv[i] == string("--generate"))
        {
            mode = operate_mode_t::generation;
        }
        else if (argv[i] == string("-t") || argv[i] == string("--db-server-path"))
        {
            if (++i > argc - 1)
            {
                cerr << c_error_prompt << "Missing path to db server." << endl;
                exit(EXIT_FAILURE);
            }
            path_to_db_server = argv[i];
        }
        else if (argv[i] == string("-o") || argv[i] == string("--output"))
        {
            if (++i > argc - 1)
            {
                cerr << c_error_prompt << "Missing path to output directory." << endl;
                exit(EXIT_FAILURE);
            }
            output_path = argv[i];
        }
        else if (argv[i] == string("-d") || argv[i] == string("--db-name"))
        {
            if (++i > argc - 1)
            {
                cerr << c_error_prompt << "Missing database name." << endl;
                exit(EXIT_FAILURE);
            }
            string db_name = argv[i];
            if (!valid_db_name(db_name))
            {
                cerr << c_error_prompt
                     << "The database name '" + db_name + "' is invalid."
                     << endl;
                exit(EXIT_FAILURE);
            }
            db_names.push_back(db_name);
        }
        else if (argv[i] == string("-h") || argv[i] == string("--help"))
        {
            cout << usage() << endl;
            exit(EXIT_SUCCESS);
        }
        else if (argv[i] == string("-v") || argv[i] == string("--version"))
        {
            cout << version() << endl;
            exit(EXIT_SUCCESS);
        }
        else if (argv[i] == string("--destroy-db"))
        {
            remove_persistent_store = true;
        }
        else
        {
            ddl_filename = argv[i];
        }
    }

    if (path_to_db_server)
    {
        server_instance_conf_t server_conf = server_instance_conf_t::get_default();
        server_conf.instance_name = c_default_instance_name;
        server_conf.server_exec_path = string(path_to_db_server) + "/" + string(c_db_server_exec_name);

        //        if (remove_persistent_store)
        //        {
        //            server_conf.reinitialize_persistent_store = true;
        //        }

        server = server_instance_t{server_conf};
        server.start();
    }

    gaia::db::begin_session();

    const auto cleanup = scope_guard::make_scope_guard([&server]() {
        gaia::db::end_session();
        if (server.is_initialized())
        {
            server.stop();
        }
    });

    if (mode == operate_mode_t::interactive)
    {
        start_repl(parser);
    }
    else
    {
        try
        {
            initialize_catalog();

            if (!ddl_filename.empty())
            {
                load_catalog(parser, ddl_filename);

                if (output_path.empty())
                {
                    output_path = filesystem::path(ddl_filename).stem();
                }
            }

            if (mode == operate_mode_t::generation)
            {
                // Generate headers for the default global database if no database is given.
                if (db_names.empty())
                {
                    db_names.emplace_back("");
                }

                for (const auto& db_name : db_names)
                {
                    generate_headers(db_name, output_path);
                }
            }
        }
        catch (gaia::common::system_error& e)
        {
            cerr << c_error_prompt << e.what() << endl;
            if (e.get_errno() == ECONNREFUSED)
            {
                cerr << "Unable to connect to the database server." << endl;
            }
            return EXIT_FAILURE;
        }
        catch (gaia_exception& e)
        {
            cerr << c_error_prompt << e.what() << endl;
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
