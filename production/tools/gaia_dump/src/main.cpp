/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////
#include <cstring>

#include "gaia_db.hpp"
#include "gaia_dump.hpp"

constexpr gaia_id_t c_high_default = 1000;
constexpr char c_references_string[] = "--references";
constexpr char c_start_string[] = "--start";
const int c_start_length = strlen(c_start_string);
constexpr char c_end_string[] = "--end";
const int c_end_length = strlen(c_end_string);
constexpr char c_line_limit_string[] = "--line-limit";
const int c_line_limit_length = strlen(c_line_limit_string);

int main(int argc, char* argv[])
{
    gaia_id_t low = 1;
    gaia_id_t high = c_high_default;
    int line_limit = -1;
    bool payload = false;
    bool references = false;
    bool catalog = false;

    // Usage:
    //  gaia_dump --start=ID --end=ID --references --payload --catalog --line-limit=N
    for (auto i = 1; i < argc; i++)
    {
        string arg(argv[i]);
        if (arg.compare(c_references_string) == 0)
        {
            references = true;
        }
        else if (arg.compare("--payload") == 0)
        {
            payload = true;
        }
        else if (arg.compare("--catalog") == 0)
        {
            catalog = true;
        }
        else if (arg.compare(0, c_start_length, c_start_string) == 0)
        {
            if (arg.length() == c_start_length)
            {
                low = atoi(argv[++i]);
            }
            else
            {
                // Allow for equals sign form (--start=12). One arg rather than two.
                low = atoi(arg.substr(c_start_length + 1).c_str());
            }
        }
        else if (arg.compare(0, c_end_length, c_end_string) == 0)
        {
            if (arg.length() == c_end_length)
            {
                high = atoi(argv[++i]);
            }
            else
            {
                // Allow for equals sign form (--end=10). One arg rather than two.
                high = atoi(arg.substr(c_end_length + 1).c_str());
            }
        }
        else if (arg.compare(0, c_line_limit_length, c_line_limit_string) == 0)
        {
            if (arg.length() == c_line_limit_length)
            {
                line_limit = atoi(argv[++i]);
            }
            else
            {
                // Allow for equals sign form (--line-limit=12). One arg rather than two.
                line_limit = atoi(arg.substr(c_line_limit_length + 1).c_str());
            }
        }
        else
        {
            fprintf(stderr, "Invalid command-line option: %s\n", argv[i]);
            fprintf(
                stderr,
                "Usage: gaia_dump [--start=ID] [--end=ID] [--references] [--payload] [--catalog] [--line-limit=N]\n");
            fprintf(stderr, "   default: gaia_dump --start=1 --end=1000\n");
            exit(1);
        }
    }

    try
    {
        gaia::db::begin_session();
    }
    catch (gaia_exception& e)
    {
        fprintf(stderr, "Startup failure, exception: %s\n", e.what());
        exit(1);
    }

    string dump = gaia_dump(low, high, payload, references, catalog, line_limit);

    gaia::db::end_session();

    cout << dump;
}
