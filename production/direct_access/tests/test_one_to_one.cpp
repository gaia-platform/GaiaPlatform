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

    person_t create_person(const char* name, const char* surname)
    {
        return person_t::get(person_t::insert_row(name, surname));
    }

    student_t create_student(const person_t& person, const char* school)
    {
        student_t student = student_t::get(person_t::insert_row(school));
        person.student().insert(student);
        return student;
    }

    employee_t create_employee(const person_t& person, const char* company)
    {
        employee_t employee = employee_t::get(employee_t::insert_row(company));
        person.employee().insert(employee);
        return employee;
    }
};

TEST_F(gaia_one_to_one_test, one_to_one)
{
    auto_transaction_t txn;

    person_t john = create_person("John", "Wick");
    person_t harry = create_person("Harry", "Potter");

    student_t student = create_student(harry, "Hogwarts");
    employee_t employee = create_employee(john, "Unclear");
}
