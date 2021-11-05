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

constexpr char c_start_string[] = "--start-after";
const int c_start_length = strlen(c_start_string);
constexpr char c_row_limit_string[] = "--row-limit";
const int c_row_limit_length = strlen(c_row_limit_string);
constexpr char c_database_string[] = "--database";
const int c_database_length = strlen(c_database_string);
constexpr char c_table_string[] = "--table";
const int c_table_length = strlen(c_table_string);

int main(int argc, char* argv[])
{
    gaia_id_t start_after = c_start_at_first;
    uint32_t row_limit = c_row_limit_unlimited;
    string database;
    string table;

    // Usage:
    //  gaia_db_extract --database=<dbmame> --table=<tablename> --start-after=ID --row-limit=N
    //  when no database/table specified, dump catalog
    for (auto i = 1; i < argc; i++)
    {
        string arg(argv[i]);

        if (!arg.compare(0, c_start_length, c_start_string))
        {
            if (arg.length() == c_start_length)
            {
                start_after = atoi(argv[++i]);
            }
            else
            {
                // Allow for equals sign form (--start-after=12). One arg rather than two.
                start_after = atoi(arg.substr(c_start_length + 1).c_str());
            }
            if (start_after < 1)
            {
                fprintf(stderr, "Illegal value for start_after. It must be 1 or greater\n");
                cout << c_empty_object << endl;
                exit(1);
            }
        }
        else if (!arg.compare(0, c_row_limit_length, c_row_limit_string))
        {
            if (arg.length() == c_row_limit_length)
            {
                row_limit = atoi(argv[++i]);
            }
            else
            {
                // Allow for equals sign form (--row-limit=12). One arg rather than two.
                row_limit = atoi(arg.substr(c_row_limit_length + 1).c_str());
            }
            if (row_limit < 1)
            {
                fprintf(stderr, "Illegal value for row_limit. It must be 1 or greater\n");
                cout << c_empty_object << endl;
                exit(1);
            }
        }
        else if (!arg.compare(0, c_database_length, c_database_string))
        {
            if (arg.length() == c_database_length)
            {
                database = string(argv[++i]);
            }
            else
            {
                // Allow for equals sign form (--row-limit=12). One arg rather than two.
                database = arg.substr(c_database_length + 1);
            }
        }
        else if (!arg.compare(0, c_table_length, c_table_string))
        {
            if (arg.length() == c_table_length)
            {
                table = string(argv[++i]);
            }
            else
            {
                // Allow for equals sign form (--row-limit=12). One arg rather than two.
                table = arg.substr(c_table_length + 1);
            }
        }
        else
        {
            fprintf(stderr, "Invalid command-row option: '%s'\n", argv[i]);
            fprintf(stderr, "Usage: gaia_db_extract [--database=<dbname>] [--table=<tableneme>] [--start-after=ID] [--row-limit=N]\n");
            cout << c_empty_object << endl;
            exit(1);
        }
    }

    try
    {
        gaia::db::begin_session();
    }
    catch (gaia_exception& e)
    {
        fprintf(stderr, "Startup failure, exception: '%s'\n", e.what());
        cout << c_empty_object << endl;
        exit(1);
    }

    // Return an empty JSON document on failure.
    string extracted_data(c_empty_object);

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
}
