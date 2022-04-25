/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia/direct_access/auto_transaction.hpp"
#include <gaia/exceptions.hpp>

#include "gaia_internal/catalog/ddl_execution.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_parser.hpp"

using namespace std;

using namespace gaia::catalog;
using namespace gaia::common;
using namespace gaia::db;

class ddl_execution_test : public db_catalog_test_base_t
{
protected:
    ddl_execution_test()
        : db_catalog_test_base_t(){};

    void SetUp() override
    {
        db_catalog_test_base_t::SetUp();

        // These tests require a DDL session,
        // so we'll be closing the session opened in db_catalog_test_base_t::SetUp().
        end_session();
        begin_ddl_session();
    }

    void check_table_name(gaia_id_t id, const string& name)
    {
        gaia::db::begin_transaction();
        gaia_table_t t = gaia_table_t::get(id);
        EXPECT_EQ(name, t.name());
        gaia::db::commit_transaction();
    }

    string get_database_hash(const string& db_name)
    {
        gaia::db::begin_transaction();
        const auto& db_list = gaia_database_t::list().where(gaia_database_expr::name == db_name);
        EXPECT_TRUE(db_list.begin() != db_list.end());
        const char* database_hash = db_list.begin()->hash();
        gaia::db::commit_transaction();
        return string(database_hash);
    }
};

TEST_F(ddl_execution_test, create_table_with_unique_constraints)
{
    ddl::parser_t parser;
    ASSERT_NO_THROW(parser.parse_string("DROP TABLE IF EXISTS t; CREATE TABLE IF NOT EXISTS t(c INT32 UNIQUE);"));
    ASSERT_EQ(parser.statements.size(), 2);
    ASSERT_NO_THROW(execute(parser.statements));

    gaia::direct_access::auto_transaction_t txn(false);
    ASSERT_TRUE(gaia_table_t::list()
                    .where(gaia_table_expr::name == "t")
                    .begin()
                    ->gaia_fields()
                    .where(gaia_field_expr::name == "c")
                    .begin()
                    ->unique());
    ASSERT_EQ(gaia_index_t::list().where(gaia_index_expr::name == "t_c").size(), 1);
}

TEST_F(ddl_execution_test, create_relationship_using_fields)
{
    const string ddl = R"(
DROP TABLE IF EXISTS t1;
CREATE TABLE IF NOT EXISTS t1(c1 INT32 UNIQUE);

DROP TABLE IF EXISTS t2;
CREATE TABLE IF NOT EXISTS t2(c2 INT32 UNIQUE);

CREATE RELATIONSHIP r1 (
  t1.link1 -> t2[],
  t2.link2 -> t1,
  USING t1(c1), t2(c2)
);
)";

    ddl::parser_t parser;
    ASSERT_NO_THROW(parser.parse_string(ddl));
    ASSERT_NO_THROW(execute(parser.statements));

    gaia::direct_access::auto_transaction_t txn(false);
    ASSERT_EQ(
        gaia_relationship_t::list().where(gaia_relationship_expr::to_child_link_name == "link1").begin()->child_field_positions()[0],
        gaia_field_t::list().where(gaia_field_expr::name == "c2").begin()->position());
    ASSERT_EQ(
        gaia_relationship_t::list().where(gaia_relationship_expr::to_child_link_name == "link1").begin()->parent_field_positions()[0],
        gaia_field_t::list().where(gaia_field_expr::name == "c1").begin()->position());
}

TEST_F(ddl_execution_test, drop_relationship)
{
    const string create_relationship_ddl = R"(
DROP TABLE IF EXISTS t1;
CREATE TABLE IF NOT EXISTS t1(c1 INT32);

DROP TABLE IF EXISTS t2;
CREATE TABLE IF NOT EXISTS t2(c2 INT32);

DROP RELATIONSHIP IF EXISTS r1;
CREATE RELATIONSHIP IF NOT EXISTS r1 (
  t1.link1 -> t2,
  t2.link2 -> t1
);

DROP RELATIONSHIP IF EXISTS r2;
CREATE RELATIONSHIP IF NOT EXISTS r2 (
  t2.link3 -> t1,
  t1.link4 -> t2
);
)";

    const string drop_relationship_ddl = R"(
DROP RELATIONSHIP IF EXISTS r1;
DROP RELATIONSHIP IF EXISTS r2;
DROP TABLE IF EXISTS t1;
DROP TABLE IF EXISTS t2;
)";

    ddl::parser_t parser;
    ASSERT_NO_THROW(parser.parse_string(create_relationship_ddl));
    ASSERT_NO_THROW(execute(parser.statements));

    {
        gaia::direct_access::auto_transaction_t txn(false);

        ASSERT_EQ(gaia_table_t::list().where(gaia_table_expr::name == "t1").size(), 1);
        ASSERT_EQ(gaia_table_t::list().where(gaia_table_expr::name == "t2").size(), 1);

        ASSERT_EQ(gaia_relationship_t::list().where(gaia_relationship_expr::name == "r1").size(), 1);
        ASSERT_EQ(gaia_relationship_t::list().where(gaia_relationship_expr::name == "r2").size(), 1);
    }

    ASSERT_NO_THROW(parser.parse_string(drop_relationship_ddl));
    ASSERT_NO_THROW(execute(parser.statements));

    {
        gaia::direct_access::auto_transaction_t txn(false);

        ASSERT_EQ(gaia_relationship_t::list().where(gaia_relationship_expr::name == "r1").size(), 0);
        ASSERT_EQ(gaia_relationship_t::list().where(gaia_relationship_expr::name == "r2").size(), 0);

        ASSERT_EQ(gaia_table_t::list().where(gaia_table_expr::name == "t1").size(), 0);
        ASSERT_EQ(gaia_table_t::list().where(gaia_table_expr::name == "t2").size(), 0);
    }

    ASSERT_NO_THROW(parser.parse_string("DROP RELATIONSHIP r3;"));
    ASSERT_THROW(execute(parser.statements), relationship_does_not_exist);
}

TEST_F(ddl_execution_test, drop_index)
{
    const string create_index_ddl = R"(
DROP TABLE IF EXISTS t;
CREATE TABLE IF NOT EXISTS t(c INT32);

DROP INDEX IF EXISTS c_i;
CREATE INDEX IF NOT EXISTS c_i ON t(c);
)";

    ddl::parser_t parser;
    ASSERT_NO_THROW(parser.parse_string(create_index_ddl));
    ASSERT_NO_THROW(execute(parser.statements));

    {
        gaia::direct_access::auto_transaction_t txn(false);
        ASSERT_EQ(gaia_table_t::list().where(gaia_table_expr::name == "t").size(), 1);
        ASSERT_EQ(
            gaia_table_t::list()
                .where(gaia::catalog::gaia_table_expr::name == "t")
                .begin()
                ->gaia_indexes()
                .where(gaia_index_expr::name == "c_i")
                .size(),
            1);
    }

    ASSERT_NO_THROW(parser.parse_string("DROP INDEX IF EXISTS c_i;"));
    ASSERT_NO_THROW(execute(parser.statements));

    {
        gaia::direct_access::auto_transaction_t txn(false);
        ASSERT_EQ(gaia_table_t::list().where(gaia_table_expr::name == "t").begin()->gaia_indexes().size(), 0);
        ASSERT_EQ(gaia_index_t::list().where(gaia_index_expr::name == "c_i").size(), 0);
    }

    ASSERT_NO_THROW(parser.parse_string("DROP INDEX c_i;"));
    ASSERT_THROW(execute(parser.statements), index_does_not_exist);
}

TEST_F(ddl_execution_test, create_list)
{
    const string create_list_ddl = R"(
CREATE RELATIONSHIP r (
  t2.link1 -> t1,
  t1.link2 -> t2
)
CREATE INDEX idx1 ON t1(c1)
CREATE TABLE t1(c1 INT32)
CREATE TABLE t2(c2 INT32);
)";
    ddl::parser_t parser;
    ASSERT_NO_THROW(parser.parse_string(create_list_ddl));
    ASSERT_NO_THROW(execute(parser.statements));
}

TEST_F(ddl_execution_test, in_table_relationship_definition)
{
    // The following list of DDLs come in pairs. The first one is the test case.
    // The second one will delete the entities created in the first. Successful
    // deletion also verifies successful creation.
    array ddls{
        // Self-reference 1:1 relationship full form.
        R"(
create table employee (
 name string,
 mentor references employee using mentee,
 mentee references employee using mentor
);
)",
        R"(
drop relationship mentee_mentor;
drop table employee;
)",
        // Self-reference 1:N relationship full form.
        R"(
create table employee (
  name string,
  manager references employee using reports,
  reports references employee[] using manager
);
)",
        R"(
drop relationship manager_reports;
drop table employee;
)",
        // Self-reference 1:N relationship without the optional 'using'.
        R"(
create table employee (
  name string,
  manager references employee,
  reports references employee[]
);
)",
        R"(
drop relationship manager_reports;
drop table employee;
)",
        // Forward reference 1:1 relationship without the optional 'using'.
        R"(
create table person (
 name string,
 employee references employee
)
create table employee (
 company string,
 person references person
);
)",
        R"(
drop relationship person_employee;
drop table person;
drop table employee;
)",
        // Forward reference 1:N relationship without the optional 'using'.
        R"(
create table doctor (
 name string,
 patients references patient[]
)
create table patient (
 name string,
 doctor references doctor
);
)",
        R"(
drop relationship doctor_patients;
drop table doctor;
drop table patient;
)",
        // Forward reference 1:N relationship with 'where' without 'using'.
        R"(
create table doctor (
 name string,
 phone_no string unique,
 patients references patient[]
)
create table patient (
 name string,
 doctor_phone_no string,
 doctor references doctor
   where patient.doctor_phone_no = doctor.phone_no
);
)",
        R"(
drop relationship doctor_patients;
drop table doctor;
drop table patient;
)",
        // Forward reference 1:N relationship with 'where'.
        R"(
create database hospital
create table doctor (
 name string,
 email string unique,
 primary_care_patients references patient[],
 secondary_care_patients references patient[]
)
create table patient (
 name string,
 primary_care_doctor_email string,
 secondary_care_doctor_email string,
 primary_care_doctor references doctor
  using primary_care_patients
  where patient.primary_care_doctor_email = doctor.email,
 secondary_care_doctor references doctor
  using secondary_care_patients
  where patient.secondary_care_doctor_email = doctor.email
);
)",
        R"(
drop relationship primary_care_doctor_primary_care_patients;
drop relationship secondary_care_doctor_secondary_care_patients;
drop database hospital;
)",
    };

    for (const auto& ddl : ddls)
    {
        ddl::parser_t parser;
        ASSERT_NO_THROW(parser.parse_string(ddl));
        ASSERT_NO_THROW(execute(parser.statements));
    }
}

TEST_F(ddl_execution_test, invalid_create_list)
{
    array ddls{
        // Table names cannot contain a database name.
        R"(
create table d.t1(c1 int32, t2 references t2)
create table d.t2(c2 int32, t1 references t1);
)",
        // Links in relationship definition cannot contain a database name.
        R"(
create database d
create table t1(c1 int32)
create table t2(c2 int32)
create relationship r (d.t1.link2 -> t2, d.t2.link1 -> t1);
)",
    };

    for (const auto& ddl : ddls)
    {
        ddl::parser_t parser;
        ASSERT_NO_THROW(parser.parse_string(ddl));
        ASSERT_THROW(execute(parser.statements), invalid_create_list);
    }
}

TEST_F(ddl_execution_test, ambiguous_reference_definition)
{
    string ddl{R"(
create table t1(c1 int32, link1a references t2, link1b references t2)
create table t2(c2 int32, link2a references t1, link2b references t1);
)"};

    ddl::parser_t parser;
    ASSERT_NO_THROW(parser.parse_string(ddl));
    ASSERT_THROW(execute(parser.statements), ambiguous_reference_definition);
}

TEST_F(ddl_execution_test, orphaned_reference_definition)
{
    string ddl{R"(
create table t1(c1 int32, link1 references t2)
create table t2(c2 int32, link2a references t1, link2b references t1);
)"};

    ddl::parser_t parser;
    ASSERT_NO_THROW(parser.parse_string(ddl));
    ASSERT_THROW(execute(parser.statements), orphaned_reference_definition);
}

TEST_F(ddl_execution_test, invalid_relationship_field)
{
    array ddls{
        // Incorrect table names in where clause.
        R"(
create table t1(c1 int32 unique, link1 references t2[])
create table t2(c2 int32, link2 references t1 where t1.c1 = t.c2);
)",
        // Field is not unique.
        R"(
create table t1(c1 int32, link1 references t2[])
create table t2(c2 int32, link2 references t1 where t1.c1 = t2.c2);
)",
        // Both fields need to be unique in 1:1 relationships.
        R"(
create table t1(c1 int32 unique, link1 references t2)
create table t2(c2 int32, link2 references t1 where t1.c1 = t2.c2);
)",
        // Non-matching where clauses.
        R"(
create table t1(a1 int16 unique, c1 int32 unique, link1 references t2[] where t1.a1 = t2.a2)
create table t2(a2 int16, c2 int32, link2 references t1 where t1.c1 = t2.c2);
)",
        // Forward reference 1:1 relationship with 'where' without 'using'.
        // We have disabled 1:1 relationships using value linked references (hybrid index).
        R"(
create table person (
 name string,
 email string unique,
 employee references employee
  where employee.personal_email = person.email
)
create table employee (
 company string,
 personal_email string unique,
 person references person
);
)",
    };

    for (const auto& ddl : ddls)
    {
        ddl::parser_t parser;
        ASSERT_NO_THROW(parser.parse_string(ddl));
        ASSERT_THROW(execute(parser.statements), invalid_relationship_field);
    }
}

TEST_F(ddl_execution_test, cross_db_relationship)
{
    string ddl =
        R"(
create database d1;
create database d2;
create table d1.t1(c1 int32);
create table d2.t2(c2 int32);
create relationship r (d1.t1.link2 -> d2.t2, d2.t2.link1 -> d1.t1);
)";

    ddl::parser_t parser;
    ASSERT_NO_THROW(parser.parse_string(ddl));
    ASSERT_THROW(execute(parser.statements), no_cross_db_relationship);
}

TEST_F(ddl_execution_test, create_optional_fields)
{
    const string ddl = R"(
DROP TABLE IF EXISTS t1;
CREATE TABLE IF NOT EXISTS t1(t1c1 INT32, t1c2 STRING OPTIONAL);

DROP TABLE IF EXISTS t2;
CREATE TABLE IF NOT EXISTS t2(t2c1 INT32 OPTIONAL UNIQUE, t2c2 INT32[] OPTIONAL);
)";

    ddl::parser_t parser;
    ASSERT_NO_THROW(parser.parse_string(ddl));
    ASSERT_NO_THROW(execute(parser.statements));

    gaia::direct_access::auto_transaction_t txn(false);
    ASSERT_EQ(gaia_field_t::list().where(gaia_field_expr::name == "t1c1").begin()->optional(), false);
    ASSERT_EQ(gaia_field_t::list().where(gaia_field_expr::name == "t1c2").begin()->optional(), true);
    ASSERT_EQ(gaia_field_t::list().where(gaia_field_expr::name == "t2c1").begin()->optional(), true);
    ASSERT_EQ(gaia_field_t::list().where(gaia_field_expr::name == "t2c2").begin()->optional(), true);
}

TEST_F(ddl_execution_test, no_system_db_operation)
{
    array ddls{
        R"(
create table catalog.t (c int32);
)",
        R"(
drop table catalog.gaia_index;
)",
        R"(
use catalog;
)",
        R"(
drop database catalog;
)",
        R"(
create database if not exists catalog
create table if not exists t (c int32);
)",
    };

    for (const auto& ddl : ddls)
    {
        ddl::parser_t parser;
        ASSERT_NO_THROW(parser.parse_string(ddl));
        ASSERT_THROW(execute(parser.statements), forbidden_system_db_operation);
    }
}

// Pairs of tables with disjoint relationships should result in hash codes
// that are the same regardless of ordering.
TEST_F(ddl_execution_test, hash_disjoint_relationships)
{
    const string ddl_order1 = R"(
CREATE TABLE t1(c1 INT32);
CREATE TABLE t2(c2 INT32);
CREATE RELATIONSHIP r1 (
  t1.link1 -> t2[],
  t2.link2 -> t1
);

CREATE TABLE t3(c3 INT32);
CREATE TABLE t4(c4 INT32);
CREATE RELATIONSHIP r2 (
  t3.link3 -> t4[],
  t4.link4 -> t3
);
)";

    const string ddl_drop = R"(
DROP RELATIONSHIP r1;
DROP RELATIONSHIP r2;
DROP TABLE IF EXISTS t1;
DROP TABLE IF EXISTS t2;
DROP TABLE IF EXISTS t3;
DROP TABLE IF EXISTS t4;
)";

    const string ddl_order2 = R"(
CREATE TABLE t4(c4 INT32);
CREATE TABLE t3(c3 INT32);
CREATE RELATIONSHIP r2 (
  t3.link3 -> t4[],
  t4.link4 -> t3
);

CREATE TABLE t2(c2 INT32);
CREATE TABLE t1(c1 INT32);
CREATE RELATIONSHIP r1 (
  t1.link1 -> t2[],
  t2.link2 -> t1
);
)";

    ddl::parser_t parser;

    // Obtain hash generated by first DDL ordering.
    ASSERT_NO_THROW(parser.parse_string(ddl_order1));
    ASSERT_NO_THROW(execute(parser.statements));
    std::string first_hash = get_database_hash(c_empty_db_name);

    // Empty the catalog definition.
    ASSERT_NO_THROW(parser.parse_string(ddl_drop));
    ASSERT_NO_THROW(execute(parser.statements));

    // Obtain hash generated by second DDL ordering.
    ASSERT_NO_THROW(parser.parse_string(ddl_order2));
    ASSERT_NO_THROW(execute(parser.statements));
    std::string second_hash = get_database_hash(c_empty_db_name);

    ASSERT_STREQ(first_hash.c_str(), second_hash.c_str());
}

// Pairs of tables with disjoint relationships should result in hash codes
// that are the same regardless of ordering.
TEST_F(ddl_execution_test, hash_disjoint_inline_relationships)
{
    const string ddl_order1 = R"(
CREATE TABLE t1(
    c1 INT32,
    t2 references t2[]
)
CREATE TABLE t2(
    c2 INT32,
    t1 references t1
)
CREATE TABLE t3(
    c3 INT32,
    t4 references t4[]
)
CREATE TABLE t4(
    c4 INT32,
    t3 references t3
);
)";

    const string ddl_drop = R"(
DROP RELATIONSHIP t1_t2;
DROP RELATIONSHIP t3_t4;
DROP TABLE IF EXISTS t1;
DROP TABLE IF EXISTS t2;
DROP TABLE IF EXISTS t3;
DROP TABLE IF EXISTS t4;
)";

    const string ddl_order2 = R"(
CREATE TABLE t3(
    c3 INT32,
    t4 references t4[]
)
CREATE TABLE t2(
    c2 INT32,
    t1 references t1
)
CREATE TABLE t1(
    c1 INT32,
    t2 references t2[]
)
CREATE TABLE t4(
    c4 INT32,
    t3 references t3
);
)";

    ddl::parser_t parser;

    // Obtain hash generated by first DDL ordering.
    ASSERT_NO_THROW(parser.parse_string(ddl_order1));
    ASSERT_NO_THROW(execute(parser.statements));
    std::string first_hash = get_database_hash(c_empty_db_name);

    // Empty the catalog definition.
    ASSERT_NO_THROW(parser.parse_string(ddl_drop));
    ASSERT_NO_THROW(execute(parser.statements));

    // Obtain hash generated by second DDL ordering.
    ASSERT_NO_THROW(parser.parse_string(ddl_order2));
    ASSERT_NO_THROW(execute(parser.statements));
    std::string second_hash = get_database_hash(c_empty_db_name);

    ASSERT_STREQ(first_hash.c_str(), second_hash.c_str());
}

// Tables with shared relationships should result in hash codes
// that are the same when the tables are in a different order.
TEST_F(ddl_execution_test, hash_overlapping_inline_relationships)
{
    const string ddl_order1 = R"(
CREATE TABLE t1(
    c1 INT32,
    t2 references t2[]
)
CREATE TABLE t2(
    c2 INT32,
    t1 references t1,
    t3 references t3
)
CREATE TABLE t3(
    c3 INT32,
    t2 references t2[]
);
)";

    const string ddl_drop = R"(
DROP RELATIONSHIP t1_t2;
DROP RELATIONSHIP t3_t2;
DROP TABLE IF EXISTS t1;
DROP TABLE IF EXISTS t2;
DROP TABLE IF EXISTS t3;
)";

    const string ddl_order2 = R"(
CREATE TABLE t2(
    c2 INT32,
    t1 references t1,
    t3 references t3
)
CREATE TABLE t3(
    c3 INT32,
    t2 references t2[]
)
CREATE TABLE t1(
    c1 INT32,
    t2 references t2[]
);
)";

    ddl::parser_t parser;

    // Obtain hash generated by first DDL ordering.
    ASSERT_NO_THROW(parser.parse_string(ddl_order1));
    ASSERT_NO_THROW(execute(parser.statements));
    std::string first_hash = get_database_hash(c_empty_db_name);

    // Empty the catalog definition.
    ASSERT_NO_THROW(parser.parse_string(ddl_drop));
    ASSERT_NO_THROW(execute(parser.statements));

    // Obtain hash generated by second DDL ordering.
    ASSERT_NO_THROW(parser.parse_string(ddl_order2));
    ASSERT_NO_THROW(execute(parser.statements));
    std::string second_hash = get_database_hash(c_empty_db_name);

    ASSERT_STRNE(first_hash.c_str(), second_hash.c_str());
}

// Tables with shared relationships should result in hash codes
// that are different when the relationships are in a different order.
TEST_F(ddl_execution_test, hash_reversed_relationships)
{
    const string ddl_order1 = R"(
CREATE TABLE t1(c1 INT32);
CREATE TABLE t2(c2 INT32);
CREATE TABLE t3(c3 INT32);
CREATE RELATIONSHIP r1 (
  t1.link1 -> t2[],
  t2.link2 -> t1
);
CREATE RELATIONSHIP r2 (
  t3.link3 -> t2[],
  t2.link4 -> t3
);
)";

    const string ddl_drop = R"(
DROP RELATIONSHIP r1;
DROP RELATIONSHIP r2;
DROP TABLE IF EXISTS t1;
DROP TABLE IF EXISTS t2;
DROP TABLE IF EXISTS t3;
)";

    const string ddl_order2 = R"(
CREATE TABLE t1(c1 INT32);
CREATE TABLE t2(c2 INT32);
CREATE TABLE t3(c3 INT32);
CREATE RELATIONSHIP r2 (
  t3.link3 -> t2[],
  t2.link4 -> t3
);
CREATE RELATIONSHIP r1 (
  t1.link1 -> t2[],
  t2.link2 -> t1
);
)";

    ddl::parser_t parser;

    // Obtain hash generated by first DDL ordering.
    ASSERT_NO_THROW(parser.parse_string(ddl_order1));
    ASSERT_NO_THROW(execute(parser.statements));
    std::string first_hash = get_database_hash(c_empty_db_name);

    // Empty the catalog definition.
    ASSERT_NO_THROW(parser.parse_string(ddl_drop));
    ASSERT_NO_THROW(execute(parser.statements));

    // Obtain hash generated by second DDL ordering.
    ASSERT_NO_THROW(parser.parse_string(ddl_order2));
    ASSERT_NO_THROW(execute(parser.statements));
    std::string second_hash = get_database_hash(c_empty_db_name);

    ASSERT_STRNE(first_hash.c_str(), second_hash.c_str());
}
