#include "gaia_gaia_u.h"
#include "gaia/system.hpp"
#include "gaia/rules/rules.hpp"
#include "gaia/direct_access/auto_transaction.hpp"


using namespace std;

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::direct_access;
using namespace gaia::gaia_u;
using namespace gaia::rules;


int main(int argc, const char**argv) {
    printf("-----------------------------------------\n");
    printf("Event Planner\n\n");
    printf("-----------------------------------------\n");
    gaia::system::initialize();

    gaia::system::shutdown();
}

void my_func(Buildings_t& building)
{
    printf("Inserted:  %s\n", building.BuildingName());

    for (auto b : Buildings_t::list())
    {
        printf("%s\n", b.BuildingName());
    }
}