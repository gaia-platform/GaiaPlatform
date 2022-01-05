/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cctype>

#include <filesystem>
#include <iostream>
#include <string>

#include <flatbuffers/idl.h>

#include "gaia/common.hpp"
#include "gaia/db/db.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/ddl_execution.hpp"
#include "gaia_internal/common/gaia_version.hpp"
#include "gaia_internal/common/logger.hpp"
#include "gaia_internal/common/scope_guard.hpp"
#include "gaia_internal/common/system_error.hpp"
#include "gaia_internal/db/db.hpp"
#include "gaia_internal/db/db_client_config.hpp"
#include "gaia_internal/db/db_server_instance.hpp"

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

string ltrim(const string& s)
{
    size_t start = s.find_first_not_of(c_whitespace_chars);
    return (start == string::npos) ? "" : s.substr(start);
}

string rtrim(const string& s)
{
    size_t end = s.find_last_not_of(c_whitespace_chars);
    return (end == string::npos) ? "" : s.substr(0, end + 1);
}

string trim(const string& s)
{
    return rtrim(ltrim(s));
}

void start_repl(parser_t& parser)
{
    initialize_catalog();

    const auto prompt = "gaiac> ";
    const auto wait_for_more_prompt = "> ";
    const auto exit_command = "exit";

    string ddl_buffer;
    while (true)
    {
        string line;
        if (ddl_buffer.empty())
        {
            cout << prompt << flush;
        }
        else
        {
            cout << wait_for_more_prompt << flush;
        }

        if (!getline(cin, line))
        {
            break;
        }
        if (trim(line) == exit_command)
        {
            break;
        }
        try
        {
            if (line.length() > 0 && ltrim(line).at(0) == c_command_prefix)
            {
                if (handle_meta_command(trim(line)))
                {
                    continue;
                }
                else
                {
                    break;
                }
            }

            if (rtrim(line).back() == ';')
            {
                parser.parse_string(ddl_buffer + line);
                execute(parser.statements);
                ddl_buffer = "";
            }
            else
            {
                ddl_buffer += line;
                ddl_buffer += '\n';
            }
        }
        catch (gaia::common::gaia_exception& e)
        {
            cerr << c_error_prompt << e.what() << endl;
            ddl_buffer = "";
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

// From the database name and catalog contents, generate the Direct Access Class definition(s).
void generate_dac_code(const string& db_name, const filesystem::path& output_path)
{
    string base_name = "gaia" + (db_name.empty() ? "" : "_" + db_name);

    filesystem::path header_path = output_path;
    header_path /= base_name + ".h";

    filesystem::path cpp_path = output_path;
    cpp_path /= base_name + ".cpp";

    ofstream dac_header(header_path);
    ofstream dac_cpp(cpp_path);
    try
    {
        dac_header << gaia::catalog::generate_dac_header(db_name) << endl;
        dac_cpp << gaia::catalog::generate_dac_cpp(db_name, header_path.filename()) << endl;
    }
    catch (gaia::common::gaia_exception& e)
    {
        cerr << "WARNING - gaia_generate failed: '" << e.what() << "'." << endl;
    }

    dac_header.close();
    dac_cpp.close();
}

void generate_edc(const string& db_name, const filesystem::path& output_path)
{
    if (output_path.empty())
    {
        cerr << "ERROR - No output location provided for the generated Direct Access files. " << endl;
        exit(1);
    }

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

    generate_fbs_headers(db_name, absolute_output_path);
    generate_dac_code(db_name, absolute_output_path);
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
    ss << "OVERVIEW: Reads Gaia DDL definitions and generates Direct Access code.\n"
          "USAGE: gaiac [options] <ddl_file>\n"
          "\n"
          "OPTIONS:\n"
          "  --db-name <dbname>     Specifies the database name to use.\n"
          "  --interactive          Run gaiac in interactive mode.\n"
          "  --generate             Generate Direct Access API header files.\n"
          "  --output <path>        Set the output directory for all generated files.\n"
#ifdef DEBUG
          "  -n, --instance-name <name> Specifies the database instance name.\n"
          "                            If not specified, will use "
       << c_default_instance_name << ".\n"
       << "                            If 'rnd' is specified will use a random name.\n"
          "  -p, --parse-trace          Print parsing trace.\n"
          "  -s, --scan-trace           Print scanning trace.\n"
          "  -t, --db-server-path       Start the DB server (for testing purposes).\n"
#endif
          "  <ddl_file>             Process the DDL statements in the file.\n"
          "  --help                 Print help information.\n"
          "  --version              Print version information.\n";
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
    string instance_name = c_default_instance_name;
    vector<string> db_names;
    string ddl_filename;
    operate_mode_t mode = operate_mode_t::loading;
    parser_t parser;
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
        else if (argv[i] == string("-n") || argv[i] == string("--instance-name"))
        {
            if (++i > argc - 1)
            {
                cerr << c_error_prompt << "Missing instance name." << endl;
                exit(EXIT_FAILURE);
            }
            instance_name = argv[i];
        }
        else if (argv[i] == string("-h") || argv[i] == string("--help"))
        {
            cerr << usage();
            exit(EXIT_SUCCESS);
        }
        else if (argv[i] == string("-v") || argv[i] == string("--version"))
        {
            cerr << version();
            exit(EXIT_SUCCESS);
        }
        else if (argv[i][0] == '-')
        {
            cerr << c_error_prompt << "Unrecognized parameter: " << argv[i] << endl;
            exit(EXIT_FAILURE);
        }
        else
        {
            ddl_filename = argv[i];

            if (!std::filesystem::exists(ddl_filename))
            {
                cerr << c_error_prompt << "DDL file '" << ddl_filename << "' does not exists." << endl;
                exit(EXIT_FAILURE);
            }
        }
    }

    // --output make sense only if gaiac is run in generation mode.
    if (!output_path.empty() && mode != operate_mode_t::generation)
    {
        cerr << c_error_prompt
             << "An output directory (--output) can be specified only in generation mode (--generate)."
             << endl;
        exit(EXIT_FAILURE);
    }

    // If an output path is not provided we generate one.
    if (mode == operate_mode_t::generation && output_path.empty())
    {
        // TODO: in the spec https://gaiaplatform.atlassian.net/browse/GAIAPLAT-1240?focusedCommentId=11101
        //  we decided to fail here, but at one day from Preview release this could be dangerous. I decided
        //  to print this warning instead.
        output_path = filesystem::current_path() / filesystem::path(ddl_filename).stem();
        cerr << c_warning_prompt
             << "Output directory (--output) not specified, using default: '" << output_path << "'"
             << endl;
    }

    // --db-name make sense only if gaiac is run in generation mode.
    if (!db_names.empty() && mode != operate_mode_t::generation)
    {
        cerr << c_error_prompt
             << "A database name (--db-name) can be specified only in generation mode (--generate)."
             << endl;
        exit(EXIT_FAILURE);
    }

    if (path_to_db_server)
    {
        server_instance_config_t server_conf = server_instance_config_t::get_new_instance_config();
        server_conf.instance_name = instance_name;
        server_conf.server_exec_path = string(path_to_db_server) + "/" + string(c_db_server_exec_name);

        server = server_instance_t{server_conf};
        server.start();
    }

    gaia::db::config::session_options_t session_options;
    session_options.db_instance_name = instance_name;
    session_options.skip_catalog_integrity_check = false;
    gaia::db::config::set_default_session_options(session_options);

    const auto server_cleanup = scope_guard::make_scope_guard(
        [&server]() {
            if (server.is_initialized())
            {
                server.stop();
            }
        });

    try
    {
        gaia::db::begin_session();
        const auto session_cleanup = scope_guard::make_scope_guard(
            []() {
                gaia::db::end_session();
            });

        if (mode == operate_mode_t::interactive)
        {
            start_repl(parser);
        }
        else
        {
            initialize_catalog();

            if (!ddl_filename.empty())
            {
                load_catalog(parser, ddl_filename);
            }

            if (mode == operate_mode_t::generation)
            {
                // Generate DAC code for the default database if no database is given.
                if (db_names.size() == 0)
                {
                    db_names.emplace_back(c_empty_db_name);
                }

                for (const auto& db_name : db_names)
                {
                    generate_edc(db_name, output_path);
                }
            }
        }
    }
    catch (gaia::common::system_error& e)
    {
        cerr << c_error_prompt << e.what() << endl;
        if (e.get_errno() == ECONNREFUSED)
        {
            cerr << "Can't connect to a running instance of the " << gaia::db::c_db_server_name << ".\n"
                 << "Start the " << gaia::db::c_db_server_name << " and rerun gaiac."
                 << endl;
        }
        return EXIT_FAILURE;
    }
    catch (gaia_exception& e)
    {
        cerr << c_error_prompt << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
