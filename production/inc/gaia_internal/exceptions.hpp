/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/exceptions.hpp"

namespace gaia
{
namespace catalog
{

class forbidden_system_db_operation_internal : public forbidden_system_db_operation
{
public:
    explicit forbidden_system_db_operation_internal(const std::string& name);
};

class db_already_exists_internal : public db_already_exists
{
public:
    explicit db_already_exists_internal(const std::string& name);
};

class db_does_not_exist_internal : public db_does_not_exist
{
public:
    explicit db_does_not_exist_internal(const std::string& name);
};

class table_already_exists_internal : public table_already_exists
{
public:
    explicit table_already_exists_internal(const std::string& name);
};

class table_does_not_exist_internal : public table_does_not_exist
{
public:
    explicit table_does_not_exist_internal(const std::string& name);
};

class duplicate_field_internal : public duplicate_field
{
public:
    explicit duplicate_field_internal(const std::string& name);
};

class field_does_not_exist_internal : public field_does_not_exist
{
public:
    explicit field_does_not_exist_internal(const std::string& name);
};

class max_reference_count_reached_internal : public max_reference_count_reached
{
public:
    explicit max_reference_count_reached_internal();
};

class referential_integrity_violation_internal : public referential_integrity_violation
{
public:
    explicit referential_integrity_violation_internal(const std::string& message);

    static referential_integrity_violation_internal drop_referenced_table(
        const std::string& referenced_table,
        const std::string& referencing_table);
};

class relationship_already_exists_internal : public relationship_already_exists
{
public:
    explicit relationship_already_exists_internal(const std::string& name);
};

class relationship_does_not_exist_internal : public relationship_does_not_exist
{
public:
    explicit relationship_does_not_exist_internal(const std::string& name);
};

class no_cross_db_relationship_internal : public no_cross_db_relationship
{
public:
    explicit no_cross_db_relationship_internal(const std::string& name);
};

class relationship_tables_do_not_match_internal : public relationship_tables_do_not_match
{
public:
    explicit relationship_tables_do_not_match_internal(
        const std::string& relationship,
        const std::string& name1,
        const std::string& name2);
};

class many_to_many_not_supported_internal : public many_to_many_not_supported
{
public:
    explicit many_to_many_not_supported_internal(const std::string& relationship);

    explicit many_to_many_not_supported_internal(const std::string& table1, const std::string& table2);
};

class index_already_exists_internal : public index_already_exists
{
public:
    explicit index_already_exists_internal(const std::string& name);
};

class index_does_not_exist_internal : public index_does_not_exist
{
public:
    explicit index_does_not_exist_internal(const std::string& name);
};

class invalid_relationship_field_internal : public invalid_relationship_field
{
public:
    explicit invalid_relationship_field_internal(const std::string& message);
};

class ambiguous_reference_definition_internal : public ambiguous_reference_definition
{
public:
    explicit ambiguous_reference_definition_internal(const std::string& table, const std::string& ref_name);
};

class orphaned_reference_definition_internal : public orphaned_reference_definition
{
public:
    explicit orphaned_reference_definition_internal(const std::string& table, const std::string& ref_name);
};

class invalid_create_list_internal : public invalid_create_list
{
public:
    explicit invalid_create_list_internal(const std::string& message);
};

} // namespace catalog

namespace common
{

class configuration_error_internal : public configuration_error
{
public:
    explicit configuration_error_internal(const char* filename);
};

class optional_value_not_found_internal : public optional_value_not_found
{
public:
    optional_value_not_found_internal();
};

namespace logging
{

class logger_exception_internal : public logger_exception
{
public:
    explicit logger_exception_internal();
};

} // namespace logging
} // namespace common

namespace db
{

class session_exists_internal : public session_exists
{
public:
    session_exists_internal();
};

class no_open_session_internal : public no_open_session
{
public:
    no_open_session_internal();
};

class transaction_in_progress_internal : public transaction_in_progress
{
public:
    transaction_in_progress_internal();
};

class no_open_transaction_internal : public no_open_transaction
{
public:
    no_open_transaction_internal();
};

class transaction_update_conflict_internal : public transaction_update_conflict
{
public:
    transaction_update_conflict_internal();
};

class transaction_object_limit_exceeded_internal : public transaction_object_limit_exceeded
{
public:
    transaction_object_limit_exceeded_internal();
};

class duplicate_object_id_internal : public duplicate_object_id
{
public:
    explicit duplicate_object_id_internal(common::gaia_id_t id);
};

class out_of_memory_internal : public out_of_memory
{
public:
    out_of_memory_internal();
};

class system_object_limit_exceeded_internal : public system_object_limit_exceeded
{
public:
    system_object_limit_exceeded_internal();
};

class invalid_object_id_internal : public invalid_object_id
{
public:
    explicit invalid_object_id_internal(common::gaia_id_t id);
};

class object_still_referenced_internal : public object_still_referenced
{
public:
    object_still_referenced_internal(
        common::gaia_id_t id, common::gaia_type_t object_type,
        common::gaia_id_t other_id, common::gaia_type_t other_type);
};

class object_too_large_internal : public object_too_large
{
public:
    object_too_large_internal(size_t total_len, uint16_t max_len);
};

class invalid_object_type_internal : public invalid_object_type
{
public:
    explicit invalid_object_type_internal(common::gaia_type_t type);

    invalid_object_type_internal(common::gaia_id_t id, common::gaia_type_t type);

    invalid_object_type_internal(
        common::gaia_id_t id,
        common::gaia_type_t expected_type,
        const char* expected_typename,
        common::gaia_type_t actual_type);
};

class session_limit_exceeded_internal : public session_limit_exceeded
{
public:
    session_limit_exceeded_internal();
};

class invalid_reference_offset_internal : public invalid_reference_offset
{
public:
    invalid_reference_offset_internal(gaia::common::gaia_type_t type, gaia::common::reference_offset_t offset);
};

class invalid_relationship_type_internal : public invalid_relationship_type
{
public:
    invalid_relationship_type_internal(
        gaia::common::reference_offset_t offset,
        gaia::common::gaia_type_t expected_type,
        gaia::common::gaia_type_t found_type);
};

class single_cardinality_violation_internal : public single_cardinality_violation
{
public:
    single_cardinality_violation_internal(gaia::common::gaia_type_t type, gaia::common::reference_offset_t offset);
};

class child_already_referenced_internal : public child_already_referenced
{
public:
    child_already_referenced_internal(gaia::common::gaia_type_t child_type, gaia::common::reference_offset_t offset);
};

class invalid_child_reference_internal : public invalid_child_reference
{
public:
    invalid_child_reference_internal(
        gaia::common::gaia_type_t child_type,
        gaia::common::gaia_id_t child_id,
        gaia::common::gaia_type_t parent_type,
        gaia::common::gaia_id_t parent_id);
};

class memory_allocation_error_internal : public memory_allocation_error
{
public:
    explicit memory_allocation_error_internal();
};

namespace index
{

class unique_constraint_violation_internal : public unique_constraint_violation
{
public:
    static constexpr char c_error_description[] = "UNIQUE constraint violation!";

public:
    // This constructor should only be used on client side,
    // to re-throw the exception indicated by the server.
    explicit unique_constraint_violation_internal(const char* error_message);

    // A violation could be triggered by conflict with a non-committed transaction.
    // If that transaction fails to commit, its record will not exist.
    // This is why no record id is being provided in the message: because it may
    // not correspond to any valid record at the time that the error is investigated.
    unique_constraint_violation_internal(const char* error_table_name, const char* error_index_name);
};

} // namespace index

} // namespace db

namespace direct_access
{

class invalid_object_state_internal : public invalid_object_state
{
public:
    invalid_object_state_internal();

    invalid_object_state_internal(
        common::gaia_id_t parent_id,
        common::gaia_id_t child_id,
        const char* child_type);
};

} // namespace direct_access

} // namespace gaia
