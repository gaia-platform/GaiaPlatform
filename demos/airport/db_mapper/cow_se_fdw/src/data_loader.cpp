/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "data_loader.h"
int main (int argc, const char ** argv)
{
    // first arg is the path to the data files
    if (argc < 2) {
        printf("Please provide a path to the airport demo data files: e.g. /gaia-platform/demos/airport/data\n");
        return 0;
    };

    AirportData demo;
    demo.init(argv[1], true);
}
