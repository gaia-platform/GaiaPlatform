#include <stdio.h>
#include <unistd.h>

#include "gaia_gaia_u.h"
#include "gaia/system.hpp"
#include "gaia/rules/rules.hpp"
#include "gaia/direct_access/auto_transaction.hpp"
#include "data_helpers.hpp"
#include "rule_helpers.hpp"
#include "loader.hpp"


using namespace std;

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::direct_access;
using namespace gaia::gaia_u;
using namespace gaia::rules;
using namespace event_planner;

void usage(const char*command) 
{
    printf("Usage: %s [-d] [-r[v] Percentage] [-l filename]\n", command);
}

int main(int argc, const char**argv) {
    printf("-----------------------------------------\n");
    printf("Event Planner\n\n");
    printf("-----------------------------------------\n");
    gaia::system::initialize("./gaia.conf");

    event_planner::is_verbose = false;

    bool show_usage = false;
    if (argc == 1)
    {
        show_all();
    }
    else
    if (argc == 2 && (strcmp(argv[1], "-d") == 0))
    {
        delete_all();
    }
    else
    if (argc == 3)
    {
        if (strcmp(argv[1], "-l") == 0)
        {
            gaia_u_loader_t loader;
            loader.load(argv[2]);
        }
        else
        if (strcmp(argv[1], "-r")== 0)
        {
            update_restriction(stoul(argv[2]));
        }
        else
        if (strcmp(argv[1], "-rv")== 0)
        {
            event_planner::is_verbose = true;
            update_restriction(stoul(argv[2]));
        }
        else
        {
            show_usage = true;
        }
    }
    else
    {
        show_usage = true;
    }

    if (show_usage) usage(argv[0]);

    
    // UNDONE:  Let the rules finish, investigate whether you have a latest edition with all the shutdown fixes ...
    // You shouldn't need this
    printf("Press any key to continue ...\n");
    getchar();
    gaia::system::shutdown();
}