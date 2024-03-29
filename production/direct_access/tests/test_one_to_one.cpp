////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <string>

#include <gtest/gtest.h>

#include "gaia_internal/db/db_catalog_test_base.hpp"
#include "gaia_internal/db/gaia_relationships.hpp"

#include "gaia_one_to_one.h"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::one_to_one;
using namespace gaia::direct_access;

class direct_access__one_to_one__test : public db_catalog_test_base_t
{
protected:
    direct_access__one_to_one__test()
        : db_catalog_test_base_t(std::string("one_to_one.ddl")){};

    template <class T_edc, typename... T_args>
    T_edc create(T_args... args)
    {
        return T_edc::get(T_edc::insert_row(args...));
    }
};

TEST_F(direct_access__one_to_one__test, connect_with_id)
{
    auto_transaction_t txn;

    auto madeline_person = create<person_t>("Madeline", "Clark");
    auto madeline_employee = create<employee_t>("Gaia Platform Authors");

    ASSERT_FALSE(madeline_person.employee());
    ASSERT_FALSE(madeline_employee.person());

    // Test connect.
    ASSERT_TRUE(madeline_person.employee().connect(madeline_employee.gaia_id()));

    ASSERT_EQ(madeline_employee.person(), madeline_person);
    ASSERT_STREQ(madeline_employee.person().name_first(), madeline_person.name_first());
    ASSERT_STREQ(madeline_employee.person().name_last(), madeline_person.name_last());

    ASSERT_EQ(madeline_person.employee(), madeline_employee);
    ASSERT_STREQ(madeline_person.employee().company(), madeline_employee.company());

    // Test disconnect.
    ASSERT_TRUE(madeline_person.employee().disconnect());

    ASSERT_FALSE(madeline_person.employee());
    ASSERT_FALSE(madeline_employee.person());
}

TEST_F(direct_access__one_to_one__test, connect_with_dac_obj)
{
    auto_transaction_t txn(auto_transaction_t::no_auto_restart);

    auto madeline_person = create<person_t>("Madeline", "Clark");
    auto madeline_employee = create<employee_t>("Gaia Platform Authors");

    ASSERT_FALSE(madeline_person.employee());
    ASSERT_FALSE(madeline_employee.person());

    txn.commit();

    // Test connect.

    // First verify that connect outside a transaction throws the expected exception.
    EXPECT_THROW(madeline_person.employee().connect(madeline_employee), no_open_transaction);

    txn.begin();

    ASSERT_TRUE(madeline_person.employee().connect(madeline_employee));

    ASSERT_EQ(madeline_employee.person(), madeline_person);
    ASSERT_STREQ(madeline_employee.person().name_first(), madeline_person.name_first());
    ASSERT_STREQ(madeline_employee.person().name_last(), madeline_person.name_last());

    ASSERT_EQ(madeline_person.employee(), madeline_employee);
    ASSERT_STREQ(madeline_person.employee().company(), madeline_employee.company());

    txn.commit();

    // Test disconnect.

    // First verify that disconnect outside a transaction throws the expected exception.
    EXPECT_THROW(madeline_person.employee().disconnect(), no_open_transaction);

    txn.begin();

    ASSERT_TRUE(madeline_person.employee().disconnect());

    ASSERT_FALSE(madeline_person.employee());
    ASSERT_FALSE(madeline_employee.person());

    txn.commit();
}

TEST_F(direct_access__one_to_one__test, multiple_disconnect_same_obj_succeed)
{
    auto_transaction_t txn;

    auto madeline_person = create<person_t>("Madeline", "Clark");

    madeline_person.employee().disconnect();
    ASSERT_FALSE(madeline_person.employee());

    auto madeline_employee = create<employee_t>("Gaia Platform Authors");
    madeline_person.employee().connect(madeline_employee);

    ASSERT_TRUE(madeline_person.employee().disconnect());
    ASSERT_FALSE(madeline_person.employee().disconnect());
    ASSERT_FALSE(madeline_person.employee());

    // Test on same ref_t object.
    auto employee = madeline_person.employee();
    employee.connect(madeline_employee);
    ASSERT_TRUE(employee.disconnect());
    ASSERT_FALSE(employee.disconnect());
    ASSERT_FALSE(employee);
}

TEST_F(direct_access__one_to_one__test, multiple_connect_same_objects_succeed)
{
    auto_transaction_t txn;

    auto madeline_person = create<person_t>("Madeline", "Clark");
    auto madeline_employee1 = create<employee_t>("Gaia Platform Authors");
    auto madeline_employee2 = create<employee_t>("Bazza LLC");

    ASSERT_TRUE(madeline_person.employee().connect(madeline_employee1));
    ASSERT_FALSE(madeline_person.employee().connect(madeline_employee1));
    ASSERT_TRUE(madeline_person.employee().connect(madeline_employee2));

    ASSERT_EQ(madeline_person.employee(), madeline_employee2);
    ASSERT_NE(madeline_person.employee(), madeline_employee1);
    ASSERT_FALSE(madeline_employee1.person());
    ASSERT_STREQ(madeline_person.employee().company(), madeline_employee2.company());

    // Test on same ref_t object.
    madeline_person.employee().disconnect();
    auto employee = madeline_person.employee();
    ASSERT_TRUE(employee.connect(madeline_employee1));
    ASSERT_FALSE(employee.connect(madeline_employee1));
    ASSERT_TRUE(employee.connect(madeline_employee2));
    ASSERT_FALSE(madeline_employee1.person());
    ASSERT_EQ(employee, madeline_employee2);
    ASSERT_EQ(madeline_person.employee(), madeline_employee2);
    employee.disconnect();
    ASSERT_FALSE(employee);
}

TEST_F(direct_access__one_to_one__test, multiple_connect_different_objects_fail)
{
    auto_transaction_t txn;

    auto madeline_person = create<person_t>("Madeline", "Clark");
    auto john_person = create<person_t>("John", "Doe");

    auto madeline_employee1 = create<employee_t>("Gaia Platform Authors");

    ASSERT_TRUE(madeline_person.employee().connect(madeline_employee1));

    EXPECT_THROW(
        john_person.employee().connect(madeline_employee1),
        child_already_referenced);
}

TEST_F(direct_access__one_to_one__test, out_of_sync_reference)
{
    auto_transaction_t txn;

    auto madeline_person = create<person_t>("Madeline", "Clark");
    auto madeline_employee1 = create<employee_t>("Gaia Platform Authors");

    auto employee = madeline_person.employee();

    madeline_person.employee().connect(madeline_employee1);

    // The employee_ref_t reference is out of sync.
    ASSERT_FALSE(employee);
}

TEST_F(direct_access__one_to_one__test, connect_wrong_type_fail)
{
    auto_transaction_t txn;

    auto madeline_person = create<person_t>("Madeline", "Clark");
    auto john_person = create<person_t>("John", "Doe");

    EXPECT_THROW(
        madeline_person.employee().connect(john_person.gaia_id()),
        invalid_relationship_type);
}

TEST_F(direct_access__one_to_one__test, connect_wrong_id_fail)
{
    auto_transaction_t txn;

    auto madeline_person = create<person_t>("Madeline", "Clark");

    employee_t empty_employee;

    EXPECT_THROW(
        madeline_person.employee().connect(empty_employee),
        invalid_object_id);
}

TEST_F(direct_access__one_to_one__test, connect_self_relationship)
{
    auto_transaction_t txn;

    auto madeline_person = create<person_t>("Madeline", "Clark");
    auto john_person = create<person_t>("John", "Doe");

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

TEST_F(direct_access__one_to_one__test, connect_multiple_relationship)
{
    auto_transaction_t txn;

    auto madeline_person = create<person_t>("Madeline", "Clark");
    auto john_person = create<person_t>("John", "Doe");

    auto madeline_employee = create<employee_t>("Gaia Platform Authors");
    auto john_employee = create<employee_t>("Bazza LLC");

    auto madeline_student = create<student_t>("UNIBO");
    auto john_student = create<student_t>("Bocconi");

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

TEST_F(direct_access__one_to_one__test, deletion)
{
    auto_transaction_t txn;

    auto madeline_person = create<person_t>("Madeline", "Clark");
    auto madeline_employee = create<employee_t>("Gaia Platform Authors");

    ASSERT_FALSE(madeline_person.employee());
    ASSERT_FALSE(madeline_employee.person());

    ASSERT_TRUE(madeline_person.employee().connect(madeline_employee.gaia_id()));

    ASSERT_NO_THROW(madeline_person.delete_row());
    ASSERT_NO_THROW(madeline_employee.delete_row());

    ASSERT_FALSE(madeline_person);
    ASSERT_FALSE(madeline_employee);
}
