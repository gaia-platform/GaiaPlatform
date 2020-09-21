/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////
#include "gaia_dump.hpp"

int main(int argc, char *argv[]) {
    gaia_id_t low = 1;
    gaia_id_t high = 1000;
    int line_limit = -1;
    bool payload = false;
    bool references = false;
    bool catalog = false;

    // Usage:
    //  gaia_dump --start=ID --end=ID --references --payload --catalog --line-limit=N
    for (auto i = 1; i < argc; i++) {
        string arg(argv[i]);
        if (arg.compare("--references") == 0) {
            references = true;
        } else if (arg.compare("--payload") == 0) {
            payload = true;
        } else if (arg.compare("--catalog") == 0) {
            catalog = true;
        } else if (arg.compare(0, 7, "--start") == 0) {
            if (arg.length() == 7) {
                low = atoi(argv[++i]);
            } else {
                low = atoi(arg.substr(8).c_str());
            }
        } else if (arg.compare(0, 5, "--end") == 0) {
            if (arg.length() == 5) {
                high = atoi(argv[++i]);
            } else {
                high = atoi(arg.substr(6).c_str());
            }
        } else if (arg.compare(0, 12, "--line-limit") == 0) {
            if (arg.length() == 12) {
                line_limit = atoi(argv[++i]);
            } else {
                line_limit = atoi(arg.substr(13).c_str());
            }
        }
        else {
            fprintf(stderr, "Invalid command-line option: %s\n", argv[i]);
            fprintf(stderr, "Usage: gaia_dump [--start=ID] [--end=ID] [--references] [--payload] [--catalog] --line-limit=N\n");
            fprintf(stderr, "   default: gaia_dump --start=1 --end=1000\n");
            exit(1);
        }
    }

    try {
        gaia::db::begin_session();
    }
    catch (gaia_exception& e) {
        fprintf(stderr, "Startup failure, exception: %s\n", e.what());
        exit(1);
    }

    string dump = gaia_dump(low, high, payload, references, catalog, line_limit);

    gaia::db::end_session();

    cout << dump;
}
