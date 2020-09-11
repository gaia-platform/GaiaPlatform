/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <fstream>
#include <iostream>
#include "gtest/gtest.h"
#include "gaia_airport.h"
#include "gaia_catalog.hpp"
#include "gaia_catalog_internal.hpp"
#include "gaia_parser.hpp"
#include "db_test_base.hpp"

using namespace gaia::catalog;
using namespace gaia::db;
using namespace std;

class gaia_dump_test : public db_test_base_t {
};

TEST_F(gaia_dump_test, placeholder) {
    EXPECT_NE(0, atoi("1"));
}
