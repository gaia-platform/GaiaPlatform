/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cstring>

#include <iostream>

#include "gaia/db/db.hpp"

#include "gaia_db_extract.hpp"

using namespace gaia::common;
using namespace gaia::tools::db_extract;
using namespace std;

constexpr char c_start_string[] = "start-after";
constexpr char c_row_limit_string[] = "row-limit";
constexpr char c_database_string[] = "database";
constexpr char c_table_string[] = "table";

// Command-line usage.
static void usage()
{
    fprintf(stderr, "Usage: gaia_db_extract [--database=<dbname>] [--table=<tableneme>] [--start-after=ID] [--row-limit=N]\n");
    fprintf(stderr, "  No parameters: dump catalog.\n");
    fprintf(stderr, "  Else dump rows specified by database/table name, limited by start-after and row-limit.\n");
}

// Break a command-line parameter into its pieces.
static bool parse_arg(int argc, char* argv[], int& arg, string& key, string& value)
{
    if (strncmp(argv[arg], "--", 2))
    {
        fprintf(stderr, "Incorrect command-line parameter: %s\n", argv[arg]);
        return false;
    }

    key = string(argv[arg] + 2);
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
            fprintf(stderr, "Missing a parameter value\n");
            return false;
        }
        value = string(argv[arg]);
    }

    return true;
}

int main(int argc, char* argv[])
{
    gaia_id_t start_after = c_start_at_first;
    uint32_t row_limit = c_row_limit_unlimited;
    string database;
    string table;

    // Usage:
    //  gaia_db_extract --database=<dbmame> --table=<tablename> --start-after=ID --row-limit=N
    //    When no database/table specified, dump catalog.
    bool failed_command_line = false;
    string key;
    string value;
    for (auto i = 1; i < argc; i++)
    {
        if (!parse_arg(argc, argv, i, key, value))
        {
            failed_command_line = true;
        }
        else
        {
            if (!key.compare(c_start_string))
            {
                start_after = atoi(value.c_str());
                if (start_after < 1)
                {
                    fprintf(stderr, "Illegal value for start_after. It must be 1 or greater\n");
                    failed_command_line = true;
                }
            }
            else if (!key.compare(c_row_limit_string))
            {
                row_limit = atoi(value.c_str());
                if (row_limit < 1)
                {
                    fprintf(stderr, "Illegal value for row_limit. It must be 1 or greater\n");
                    failed_command_line = true;
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
                fprintf(stderr, "Invalid command-line option: '%s'\n", key.c_str());
                failed_command_line = true;
            }
        }
    }

    if (failed_command_line)
    {
        usage();
        cout << "{}" << endl;
        exit(EXIT_FAILURE);
    }

    try
    {
        gaia::db::begin_session();
    }
    catch (gaia_exception& e)
    {
        // This is usually because there is no server running.
        fprintf(stderr, "Startup failure, exception: '%s'\n", e.what());
        cout << "{}" << endl;
        exit(EXIT_FAILURE);
    }

    // Return an empty JSON document on failure.
    string extracted_data("{}");

    try
    {
        // One-time preparation for scanning rows.
        if (!gaia_db_extract_initialize())
        {
            fprintf(stderr, "Extraction API failed to initialize\n");
        }
        else
        {
            extracted_data = gaia_db_extract(database, table, start_after, row_limit);
        }
    }
    catch (gaia_exception& e)
    {
        fprintf(stderr, "Exception while generating JSON: '%s'\n", e.what());
    }

    gaia::db::end_session();

    cout << extracted_data << endl;

    exit(EXIT_SUCCESS);
}
