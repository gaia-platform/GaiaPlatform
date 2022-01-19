/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/exceptions.hpp"

namespace gaia
{
namespace db
{

session_exists_internal::session_exists_internal()
{
    m_message = "Close the current session before opening a new one.";
}

no_open_session_internal::no_open_session_internal()
{
    m_message = "Open a session before performing data access.";
}

transaction_in_progress_internal::transaction_in_progress_internal()
{
    m_message = "Commit or rollback the current transaction before opening a new transaction.";
}

no_open_transaction_internal::no_open_transaction_internal()
{
    m_message = "Open a transaction before performing data access.";
}

transaction_update_conflict_internal::transaction_update_conflict_internal()
{
    m_message = "Transaction was aborted due to a conflict with another transaction.";
}

transaction_object_limit_exceeded_internal::transaction_object_limit_exceeded_internal()
{
    m_message = "Transaction attempted to update too many objects.";
}

duplicate_object_id_internal::duplicate_object_id_internal(common::gaia_id_t id)
{
    std::stringstream message;
    message << "An object with the same ID '" << id << "' already exists.";
    m_message = message.str();
}

out_of_memory_internal::out_of_memory_internal()
{
    m_message = "Out of memory.";
}

system_object_limit_exceeded_internal::system_object_limit_exceeded_internal()
{
    m_message = "System object limit exceeded.";
}

invalid_object_id_internal::invalid_object_id_internal(common::gaia_id_t id)
{
    std::stringstream message;
    message << "Cannot find an object with ID '" << id << "'.";
    m_message = message.str();
}

object_still_referenced_internal::object_still_referenced_internal(
    common::gaia_id_t id, common::gaia_type_t object_type,
    common::gaia_id_t other_id, common::gaia_type_t other_type)
{
    std::stringstream message;
    message
        << "Cannot delete object with ID '" << id << "', type '" << object_type
        << "', because it is still referenced by another object with ID '"
        << other_id << "', type '" << other_type << "'. "
        << "Use the 'force' option to force delete the object.";
    m_message = message.str();
}

object_too_large_internal::object_too_large_internal(size_t total_len, uint16_t max_len)
{
    std::stringstream message;
    message << "Object size " << total_len << " exceeds maximum size " << max_len << ".";
    m_message = message.str();
}

invalid_object_type_internal::invalid_object_type_internal(common::gaia_type_t type)
{
    std::stringstream message;
    message << "The type '" << type << "' does not exist in the catalog.";
    m_message = message.str();
}

invalid_object_type_internal::invalid_object_type_internal(common::gaia_id_t id, common::gaia_type_t type)
{
    std::stringstream message;
    message
        << "Cannot create object with ID '" << id << "' and type '" << type
        << "'. The type does not exist in the catalog.";
    m_message = message.str();
}

invalid_object_type_internal::invalid_object_type_internal(
    common::gaia_id_t id,
    common::gaia_type_t expected_type,
    const char* expected_typename,
    common::gaia_type_t actual_type)
{
    std::stringstream message;
    message
        << "Requesting Gaia type '" << expected_typename << "'('" << expected_type
        << "'), but object identified by '" << id << "' is of type '" << actual_type << "'.";
    m_message = message.str();
}

session_limit_exceeded_internal::session_limit_exceeded_internal()
{
    m_message = "Server session limit exceeded.";
}

invalid_reference_offset_internal::invalid_reference_offset_internal(
    gaia::common::gaia_type_t type, gaia::common::reference_offset_t offset)
{
    std::stringstream message;
    message << "Gaia type '" << type << "' has no relationship for the offset '" << offset << "'.";
    m_message = message.str();
}

invalid_relationship_type_internal::invalid_relationship_type_internal(
    gaia::common::reference_offset_t offset,
    gaia::common::gaia_type_t expected_type,
    gaia::common::gaia_type_t found_type)
{
    std::stringstream message;
    message
        << "Relationship with offset '" << offset << "' requires type '" << expected_type
        << "' but found type '" << found_type << "'.";
    m_message = message.str();
}

single_cardinality_violation_internal::single_cardinality_violation_internal(gaia::common::gaia_type_t type, gaia::common::reference_offset_t offset)
{
    std::stringstream message;
    message
        << "Gaia type '" << type << "' has single cardinality for the relationship with offset '" << offset
        << "', but multiple children are being added.";
    m_message = message.str();
}

child_already_referenced_internal::child_already_referenced_internal(gaia::common::gaia_type_t child_type, gaia::common::reference_offset_t offset)
{
    std::stringstream message;
    message
        << "Gaia type '" << child_type
        << "' already has a reference for the relationship with offset '" << offset << "'.";
    m_message = message.str();
}

invalid_child_reference_internal::invalid_child_reference_internal(
    gaia::common::gaia_type_t child_type,
    gaia::common::gaia_id_t child_id,
    gaia::common::gaia_type_t parent_type,
    gaia::common::gaia_id_t parent_id)
{
    std::stringstream message;
    message
        << "Cannot remove child with id '" << child_id
        << "' and type '" << child_type
        << "' from parent with id '" << parent_id
        << "' and type '" << parent_type << "'. The child has a different parent.";
    m_message = message.str();
}

memory_allocation_error_internal::memory_allocation_error_internal()
{
    m_message = "The Gaia database ran out of memory.";
}

namespace index
{

unique_constraint_violation_internal::unique_constraint_violation_internal(const char* error_message)
{
    m_message = error_message;
}

unique_constraint_violation_internal::unique_constraint_violation_internal(const char* error_table_name, const char* error_index_name)
{
    std::stringstream message;
    message
        << c_error_description
        << " Cannot insert a duplicate key in table: '"
        << error_table_name << "', because of the unique constraint of "
        << " index: '" << error_index_name << "'.";
    m_message = message.str();
}

} // namespace index

} // namespace db
} // namespace gaia
