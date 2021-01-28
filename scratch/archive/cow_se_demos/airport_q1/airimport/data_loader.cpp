/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "data_loader.hpp"
int main (int argc, const char ** argv)
{
    // The first arg is the path to the data files.
    if (argc < 2) {
        printf("Please provide a path to the airport demo data files: i.e. /home/dax/git/benchmarks/airimport/data/\n");
        return 0;
    };

    AirportData demo;
    demo.init(argv[1]);
}
