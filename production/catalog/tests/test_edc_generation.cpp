///////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"

#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "addr_book.h"
#include "gaia_generate.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::catalog::generate;
using namespace gaia::addr_book;
using namespace gaia::addr_book::employee_expr;

class test_edc_generation : public db_catalog_test_base_t
{
protected:
    test_edc_generation()
        : db_catalog_test_base_t("addr_book.ddl"){};
};

TEST_F(test_edc_generation, generation)
{
    begin_transaction();
    gaia_edc_generator_t gaia_edc_generator{"addr_book"};
    gaia_edc_generator.generate(
        "/home/simone/repos/GaiaPlatform/production/catalog/tests/addr_book.h",
        "/home/simone/repos/GaiaPlatform/production/catalog/tests/addr_book.cpp");
}

TEST_F(test_edc_generation, generation2)
{
    begin_transaction();
    employee_writer w;
    w.name_first = "Boppini";
    w.name_last = "Olbudio";
    w.insert_row();

    auto itr = employee_t::list().where(name_first == "Boppini");
    ASSERT_STREQ(itr.begin()->name_first(), "Boppini");
}
