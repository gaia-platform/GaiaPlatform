/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include any internal headers to ensure that
// the code included in this file doesn't have a dependency
// on non-public APIs.

#include "test_sdk_base.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::rules;
using namespace gaia::addr_book;
using namespace gaia::db::triggers;

class sdk_db__test : public sdk_base
{
};

TEST_F(sdk_db__test, auto_txn)
{
    auto_transaction_t tx;
    employee_writer w;
    w.name_first = "Public";
    w.name_last = "Headers";
    gaia_id_t id = w.insert_row();
    employee_t e = employee_t::get(id);
    e.delete_row();
    tx.commit();
}

TEST_F(sdk_db__test, gaia_logger)
{
    static constexpr char c_const_char_msg[] = "const char star message";
    static const std::string c_string_msg = "string message";
    static constexpr int64_t c_int_msg = 1234;

    gaia_log::app().trace("trace");
    gaia_log::app().trace("trace const char*: '{}', std::string: '{}', number: '{}'", c_const_char_msg, c_string_msg, c_int_msg);
    gaia_log::app().debug("debug");
    gaia_log::app().debug("debug const char*: '{}', std::string: '{}', number: '{}'", c_const_char_msg, c_string_msg, c_int_msg);
    gaia_log::app().info("info");
    gaia_log::app().info("info const char*: '{}', std::string: '{}', number: '{}'", c_const_char_msg, c_string_msg, c_int_msg);
    gaia_log::app().warn("warn");
    gaia_log::app().warn("warn const char*: '{}', std::string: '{}', number: '{}'", c_const_char_msg, c_string_msg, c_int_msg);
    gaia_log::app().error("error");
    gaia_log::app().error("error const char*: '{}', std::string: '{}', number: '{}'", c_const_char_msg, c_string_msg, c_int_msg);
    gaia_log::app().critical("critical");
    gaia_log::app().critical("critical const char*: '{}', std::string: '{}', number: '{}'", c_const_char_msg, c_string_msg, c_int_msg);
}

TEST_F(sdk_db__test, transactions)
{
    EXPECT_FALSE(gaia::db::is_transaction_open());

    gaia::db::begin_transaction();
    EXPECT_TRUE(gaia::db::is_transaction_open());
    gaia::db::commit_transaction();
    EXPECT_FALSE(gaia::db::is_transaction_open());

    gaia::db::begin_transaction();
    EXPECT_TRUE(gaia::db::is_transaction_open());
    gaia::db::rollback_transaction();
    EXPECT_FALSE(gaia::db::is_transaction_open());
}

// The tests below ensure that all public exceptions can
// be referenced without compile or link errors.

// Catalog exceptions.
TEST_F(sdk_db__test, catalog_exceptions)
{
    test_exception<gaia::catalog::forbidden_system_db_operation>();
    test_exception<gaia::catalog::db_already_exists>();
    test_exception<gaia::catalog::db_does_not_exist>();
    test_exception<gaia::catalog::table_already_exists>();
    test_exception<gaia::catalog::table_does_not_exist>();
    test_exception<gaia::catalog::duplicate_field>();
    test_exception<gaia::catalog::field_does_not_exist>();
    test_exception<gaia::catalog::max_reference_count_reached>();
    test_exception<gaia::catalog::referential_integrity_violation>();
    test_exception<gaia::catalog::relationship_already_exists>();
    test_exception<gaia::catalog::relationship_does_not_exist>();
    test_exception<gaia::catalog::no_cross_db_relationship>();
    test_exception<gaia::catalog::relationship_tables_do_not_match>();
    test_exception<gaia::catalog::many_to_many_not_supported>();
    test_exception<gaia::catalog::index_already_exists>();
    test_exception<gaia::catalog::index_does_not_exist>();
    test_exception<gaia::catalog::invalid_relationship_field>();
    test_exception<gaia::catalog::ambiguous_reference_definition>();
    test_exception<gaia::catalog::orphaned_reference_definition>();
    test_exception<gaia::catalog::invalid_create_list>();
}

// Common exceptions.
TEST_F(sdk_db__test, common_exceptions)
{
    test_exception<gaia::common::configuration_error>();
    test_exception<gaia::common::logging::logger_exception>();
}

// Database exceptions.
TEST_F(sdk_db__test, db_exceptions)
{
    test_exception<gaia::db::session_exists>();
    test_exception<gaia::db::no_open_session>();
    test_exception<gaia::db::transaction_in_progress>();
    test_exception<gaia::db::no_open_transaction>();
    test_exception<gaia::db::transaction_update_conflict>();
    test_exception<gaia::db::transaction_object_limit_exceeded>();
    test_exception<gaia::db::duplicate_object_id>();
    test_exception<gaia::db::out_of_memory>();
    test_exception<gaia::db::system_object_limit_exceeded>();
    test_exception<gaia::db::invalid_object_id>();
    test_exception<gaia::db::object_still_referenced>();
    test_exception<gaia::db::object_too_large>();
    test_exception<gaia::db::invalid_object_type>();
    test_exception<gaia::db::session_limit_exceeded>();
    test_exception<gaia::db::invalid_reference_offset>();
    test_exception<gaia::db::invalid_relationship_type>();
    test_exception<gaia::db::single_cardinality_violation>();
    test_exception<gaia::db::child_already_referenced>();
    test_exception<gaia::db::invalid_child_reference>();
    test_exception<gaia::db::memory_allocation_error>();
    test_exception<gaia::db::pre_commit_validation_failure>();
    test_exception<gaia::db::index::unique_constraint_violation>();
}

// Direct access exceptions.
TEST_F(sdk_db__test, direct_access_exceptions)
{
    test_exception<gaia::direct_access::invalid_object_state>();
}
