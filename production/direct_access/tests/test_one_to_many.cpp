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

#include "gaia_addr_book.h"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::addr_book;
using namespace gaia::direct_access;

class direct_access__one_to_many__test : public db_catalog_test_base_t
{
protected:
    direct_access__one_to_many__test()
        : db_catalog_test_base_t(std::string("addr_book.ddl")){};

    employee_t insert_employee(const std::string& name, const std::string& surname)
    {
        employee_writer ew;
        ew.name_first = name.c_str();
        ew.name_last = surname.c_str();
        return employee_t::get(ew.insert_row());
    }

    address_t insert_address(const std::string& city)
    {
        address_writer aw;
        aw.city = city.c_str();
        return address_t::get(aw.insert_row());
    }
};

TEST_F(direct_access__one_to_many__test, connect_with_dac_obj)
{
    auto_transaction_t txn;

    auto madeline = insert_employee("Madeline", "Clark");
    auto seattle = insert_address("Seattle");

    ASSERT_TRUE(madeline.addresses().connect(seattle));
}

TEST_F(direct_access__one_to_many__test, multiple_disconnect_succeed)
{
    auto_transaction_t txn;

    auto madeline = insert_employee("Madeline", "Clark");
    auto seattle = insert_address("Seattle");
    madeline.addresses().connect(seattle);

    ASSERT_TRUE(madeline.addresses().disconnect(seattle));
    ASSERT_FALSE(madeline.addresses().disconnect(seattle));
}

TEST_F(direct_access__one_to_many__test, multiple_disconnect_different_object_fail)
{
    auto_transaction_t txn;

    auto madeline = insert_employee("Madeline", "Clark");
    auto seattle = insert_address("Seattle");
    madeline.addresses().connect(seattle);

    ASSERT_TRUE(madeline.addresses().disconnect(seattle));

    auto john = insert_employee("John", "Doe");
    ASSERT_FALSE(john.addresses().disconnect(seattle));
}

TEST_F(direct_access__one_to_many__test, multiple_connect_same_obj_succeed)
{
    auto_transaction_t txn;

    auto madeline = insert_employee("Madeline", "Clark");
    auto kissimmee = insert_address("Kissimmee");

    ASSERT_TRUE(madeline.addresses().connect(kissimmee));
    ASSERT_FALSE(madeline.addresses().connect(kissimmee));
}

TEST_F(direct_access__one_to_many__test, multiple_connect_different_obj_fail)
{
    auto_transaction_t txn;

    auto madeline = insert_employee("Madeline", "Clark");
    auto kissimmee = insert_address("Kissimmee");
    ASSERT_TRUE(madeline.addresses().connect(kissimmee));

    auto john = insert_employee("John", "Doe");
    ASSERT_THROW(
        john.addresses().connect(kissimmee),
        child_already_referenced);
}
