/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cstring>

#include <iostream>

#include "gaia/db/db.hpp"
#include "gaia/exceptions.hpp"

#include "gaia_db_extract.hpp"

using namespace gaia::common;
using namespace gaia::tools::db_extract;
using namespace std;

constexpr char c_start_after_string[] = "start-after";
constexpr char c_row_limit_string[] = "row-limit";
constexpr char c_help_string[] = "help";
constexpr char c_database_string[] = "database";
constexpr char c_table_string[] = "table";

// Command-line usage.
static void usage()
{
    cerr << "OVERVIEW: Outputs a JSON representation of either the table schema, or selected rows within a database and table." << endl
         << endl;
    cerr << "USAGE: gaia_db_extract [--" << c_help_string << "] [--" << c_database_string << "=<databasename> --" << c_table_string
         << "=<tablename> [--" << c_row_limit_string << "=N] [--" << c_start_after_string << "=ID]]" << endl
         << endl;
    cerr << "NO OPTIONS: extract table schema information only." << endl
         << endl;
    cerr << "OPTIONS TO EXTRACT ROWS:" << endl;
    cerr << "  --" << c_database_string << "=<databasename>    Required. Selects the database from which to extract rows." << endl;
    cerr << "  --" << c_table_string << "=<tablename>          Required. Selects the table containing the rows being extracted." << endl;
    cerr << "  --" << c_row_limit_string << "=N                Optional. Limit the number of rows to N. Otherwise, extract all rows." << endl;
    cerr << "  --" << c_start_after_string << "=ID             Optional. Specifies the ID of the row that precedes the row at which" << endl;
    cerr << "                               to start the extract. Typically this will be the last row of the" << endl;
    cerr << "                               block that you just extracted. To extract rows from the database" << endl;
    cerr << "                               in blocks, use this parameter in conjunction with row-limit." << endl;
    cerr << "                               Omit this parameter to start from the beginning." << endl;

    // Print an empty JSON object when there is any kind of error.
    cout << "{}" << endl;

    exit(EXIT_FAILURE);
}

// Break a command-line parameter into its pieces.
static void parse_arg(int argc, char* argv[], int& arg, string& key, string& value)
{
    if (strncmp(argv[arg], "--", 2))
    {
        cerr << "Unrecognized parameter: " << argv[arg] << endl
             << endl;
        usage();
    }

    key = string(argv[arg] + 2);
    if (!key.compare(c_help_string))
    {
        usage();
    }

    auto pos = key.find('=');
    if (pos != string::npos)
    {
        // It's one argv with an equals sign.
        value = key.substr(pos + 1);
        key = key.substr(0, pos);
    }
    else
    {
        // It's two argv's, and the value is the next one.
        if (++arg >= argc)
        {
            cerr << "Missing a parameter value." << endl
                 << endl;
            usage();
        }
        value = string(argv[arg]);
    }
}

int main(int argc, char* argv[])
{
    gaia_id_t start_after = c_start_at_first;
    uint32_t row_limit = c_row_limit_unlimited;
    string database;
    string table;

    string key;
    string value;
    for (auto i = 1; i < argc; i++)
    {
        // Get a parameter and value pair from the command-line.
        parse_arg(argc, argv, i, key, value);

        if (!key.compare(c_start_after_string))
        {
            start_after = stoi(value);
            if (start_after < 1)
            {
                cerr << "Illegal value for " << c_start_after_string << ". It must be 1 or greater." << endl
                     << endl;
                usage();
            }
        }
        else if (!key.compare(c_row_limit_string))
        {
            row_limit = stoi(value);
            if (row_limit < 1)
            {
                cerr << "Illegal value for " << c_row_limit_string << ". It must be 1 or greater." << endl
                     << endl;
                usage();
            }
        }
        else if (!key.compare(c_database_string))
        {
            database = value;
        }
        else if (!key.compare(c_table_string))
        {
            table = value;
        }
        else
        {
            cerr << "Unrecognized parameter: '" << key << "'." << endl
                 << endl;
            usage();
        }
    }

    try
    {
        gaia::db::begin_session();
    }
    catch (gaia_exception& e)
    {
        // This is usually because there is no server running.
        cerr << "Startup failure, exception: '" << e.what() << "'." << endl;
        cout << "{}" << endl;
        exit(EXIT_FAILURE);
    }

    // Return an empty JSON document on failure.
    string extracted_data(c_empty_object);

    try
    {
        // One-time preparation for scanning rows.
        if (!gaia_db_extract_initialize())
        {
            cerr << "Extraction API failed to initialize." << endl;
        }
        else
        {
            extracted_data = gaia_db_extract(database, table, start_after, row_limit);
        }
    }
    catch (gaia_exception& e)
    {
        cerr << "Exception while generating JSON: '" << e.what() << "'." << endl;
        gaia::db::rollback_transaction();
    }

    gaia::db::end_session();

    cout << extracted_data << endl;

    exit(EXIT_SUCCESS);
}
