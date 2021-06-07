/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <string>

#include <gtest/gtest.h>

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_one_to_one.h"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::one_to_one;
using namespace gaia::direct_access;

class gaia_one_to_one_test : public db_catalog_test_base_t
{
protected:
    gaia_one_to_one_test()
        : db_catalog_test_base_t(std::string("one_to_one.ddl")){};

    template <class T_edc, typename... T_args>
    T_edc create(T_args... args)
    {
        return T_edc::get(T_edc::insert_row(args...));
    }
};

TEST_F(gaia_one_to_one_test, one_to_one_with_id)
{
    auto_transaction_t txn;

    person_t madeline_person = create<person_t>("Madeline", "Clark");
    employee_t madeline_employee = create<employee_t>("Gaia Platform LLC");

    ASSERT_FALSE(madeline_person.employee());
    ASSERT_FALSE(madeline_employee.person());

    // Test connect.
    madeline_person.employee().connect(madeline_employee.gaia_id());

    ASSERT_EQ(madeline_employee.person(), madeline_person);
    ASSERT_STREQ(madeline_employee.person().name_first(), madeline_person.name_first());
    ASSERT_STREQ(madeline_employee.person().name_last(), madeline_person.name_last());

    ASSERT_EQ(madeline_person.employee(), madeline_employee);
    ASSERT_STREQ(madeline_person.employee().company(), madeline_employee.company());

    // Test disconnect.
    madeline_person.employee().disconnect(madeline_employee.gaia_id());

    ASSERT_FALSE(madeline_person.employee());
    ASSERT_FALSE(madeline_employee.person());
}

//TEST_F(gaia_one_to_one_test, one_to_one)
//{
//    auto_transaction_t txn;
//
//    person_t madeline_person = create<person_t>("Madeline", "Clark");
//    person_t joel_person = create<person_t>("Joel", "Phelps");
//
//    ASSERT_FALSE(madeline_person.employee());
//    ASSERT_FALSE(joel_person.employee());
//
//    employee_t madeline_employee = create<employee_t>("Gaia Platform LLC");
//    employee_t joel_employee = create<employee_t>("Bazza LLC");
//
//    ASSERT_FALSE(madeline_employee.person());
//    ASSERT_FALSE(joel_employee.person());
//
//    madeline_person.employee().connect(madeline_employee.gaia_id());
//    madeline_person.employee().connect(madeline_employee.gaia_id());
//
//}
