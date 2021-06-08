/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <string>

#include <gtest/gtest.h>

#include <gaia_internal/db/db_catalog_test_base.hpp>
#include <gaia_internal/db/gaia_relationships.hpp>

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

TEST_F(gaia_one_to_one_test, connect_with_id)
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
    madeline_person.employee().disconnect();

    ASSERT_FALSE(madeline_person.employee());
    ASSERT_FALSE(madeline_employee.person());
}

TEST_F(gaia_one_to_one_test, connect_with_edc_obj)
{
    auto_transaction_t txn;

    person_t madeline_person = create<person_t>("Madeline", "Clark");
    employee_t madeline_employee = create<employee_t>("Gaia Platform LLC");

    ASSERT_FALSE(madeline_person.employee());
    ASSERT_FALSE(madeline_employee.person());

    // Test connect.
    madeline_person.employee().connect(madeline_employee);

    ASSERT_EQ(madeline_employee.person(), madeline_person);
    ASSERT_STREQ(madeline_employee.person().name_first(), madeline_person.name_first());
    ASSERT_STREQ(madeline_employee.person().name_last(), madeline_person.name_last());

    ASSERT_EQ(madeline_person.employee(), madeline_employee);
    ASSERT_STREQ(madeline_person.employee().company(), madeline_employee.company());

    // Test disconnect.
    madeline_person.employee().disconnect();

    ASSERT_FALSE(madeline_person.employee());
    ASSERT_FALSE(madeline_employee.person());
}

TEST_F(gaia_one_to_one_test, mutliple_disconnect_succeed)
{
    auto_transaction_t txn;

    person_t madeline_person = create<person_t>("Madeline", "Clark");

    madeline_person.employee().disconnect();
    ASSERT_FALSE(madeline_person.employee());

    employee_t madeline_employee = create<employee_t>("Gaia Platform LLC");
    madeline_person.employee().connect(madeline_employee);

    madeline_person.employee().disconnect();
    madeline_person.employee().disconnect();
    ASSERT_FALSE(madeline_person.employee());
}

TEST_F(gaia_one_to_one_test, mutliple_connect_succeed)
{
    auto_transaction_t txn;

    person_t madeline_person = create<person_t>("Madeline", "Clark");
    employee_t madeline_employee1 = create<employee_t>("Gaia Platform LLC");
    employee_t madeline_employee2 = create<employee_t>("Bazza LLC");

    madeline_person.employee().connect(madeline_employee1);
    madeline_person.employee().connect(madeline_employee2);

    ASSERT_EQ(madeline_person.employee(), madeline_employee2);
    ASSERT_STREQ(madeline_person.employee().company(), madeline_employee2.company());
}

TEST_F(gaia_one_to_one_test, connect_wrong_type_fail)
{
    auto_transaction_t txn;

    person_t madeline_person = create<person_t>("Madeline", "Clark");
    person_t john_person = create<person_t>("John", "Doe");

    EXPECT_THROW(
        madeline_person.employee().connect(john_person.gaia_id()),
        invalid_relationship_type);
}

TEST_F(gaia_one_to_one_test, connect_wrong_id_fail)
{
    auto_transaction_t txn;

    person_t madeline_person = create<person_t>("Madeline", "Clark");
    person_t john_person = create<person_t>("John", "Doe");

    employee_t empty_employee;

    EXPECT_THROW(
        madeline_person.employee().connect(empty_employee),
        invalid_object_id);
}

TEST_F(gaia_one_to_one_test, connect_self_relationship)
{
    auto_transaction_t txn;

    person_t madeline_person = create<person_t>("Madeline", "Clark");
    person_t john_person = create<person_t>("John", "Doe");

    madeline_person.husband().connect(john_person);

    ASSERT_EQ(madeline_person.husband(), john_person);
    ASSERT_FALSE(madeline_person.wife());

    ASSERT_EQ(john_person.wife(), madeline_person);
    ASSERT_FALSE(john_person.husband());

    madeline_person.husband().disconnect();

    ASSERT_FALSE(madeline_person.husband());
    ASSERT_FALSE(john_person.wife());

    // Try a self-self relationship
    madeline_person.husband().connect(madeline_person);
    ASSERT_EQ(madeline_person.husband(), madeline_person);
    ASSERT_EQ(madeline_person.wife(), madeline_person);

    madeline_person.husband().disconnect();
    ASSERT_FALSE(john_person.husband());
    ASSERT_FALSE(john_person.wife());
}

TEST_F(gaia_one_to_one_test, connect_multiple_relationship)
{
    auto_transaction_t txn;

    person_t madeline_person = create<person_t>("Madeline", "Clark");
    person_t john_person = create<person_t>("John", "Doe");

    employee_t madeline_employee = create<employee_t>("Gaia Platform LLC");
    employee_t john_employee = create<employee_t>("Bazza LLC");

    student_t madeline_student = create<student_t>("UNIBO");
    student_t john_student = create<student_t>("Bocconi");

    madeline_person.employee().connect(madeline_employee);
    madeline_person.student().connect(madeline_student);
    john_person.employee().connect(john_employee);
    john_person.student().connect(john_student);
    madeline_person.husband().connect(john_person);

    ASSERT_EQ(madeline_person.husband(), john_person);
    ASSERT_EQ(madeline_person.student(), madeline_student);
    ASSERT_EQ(madeline_person.employee(), madeline_employee);

    ASSERT_EQ(john_person.wife(), madeline_person);
    ASSERT_EQ(john_person.student(), john_student);
    ASSERT_EQ(john_person.employee(), john_employee);
}
