/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia/exceptions.hpp"

namespace gaia
{
namespace db
{

session_exists::session_exists()
{
    m_message = "Close the current session before opening a new one.";
}

no_open_session::no_open_session()
{
    m_message = "Open a session before performing data access.";
}

transaction_in_progress::transaction_in_progress()
{
    m_message = "Commit or rollback the current transaction before opening a new transaction.";
}

no_open_transaction::no_open_transaction()
{
    m_message = "Open a transaction before performing data access.";
}

transaction_update_conflict::transaction_update_conflict()
{
    m_message = "Transaction was aborted due to a conflict with another transaction.";
}

transaction_object_limit_exceeded::transaction_object_limit_exceeded()
{
    m_message = "Transaction attempted to update too many objects.";
}

duplicate_id::duplicate_id(common::gaia_id_t id)
{
    std::stringstream strs;
    strs << "An object with the same ID '" << id << "' already exists.";
    m_message = strs.str();
}

out_of_memory::out_of_memory()
{
    m_message = "Out of memory.";
}

system_object_limit_exceeded::system_object_limit_exceeded()
{
    m_message = "System object limit exceeded.";
}

invalid_object_id::invalid_object_id(common::gaia_id_t id)
{
    std::stringstream message;
    message << "Cannot find an object with ID '" << id << "'.";
    m_message = message.str();
}

object_still_referenced::object_still_referenced(
    common::gaia_id_t id, common::gaia_type_t object_type,
    common::gaia_id_t other_id, common::gaia_type_t other_type)
{
    std::stringstream message;
    message
        << "Cannot delete object with ID '" << id << "', type '" << object_type
        << "', because it is still referenced by another object with ID '"
        << other_id << "', type '" << other_type << "'";
    m_message = message.str();
}

object_too_large::object_too_large(size_t total_len, uint16_t max_len)
{
    std::stringstream message;
    message << "Object size " << total_len << " exceeds maximum size " << max_len << ".";
    m_message = message.str();
}

invalid_type::invalid_type(common::gaia_type_t type)
{
    std::stringstream message;
    message << "The type '" << type << "' does not exist in the catalog.";
    m_message = message.str();
}

invalid_type::invalid_type(common::gaia_id_t id, common::gaia_type_t type)
{
    std::stringstream message;
    message
        << "Cannot create object with ID '" << id << "' and type '" << type
        << "'. The type does not exist in the catalog.";
    m_message = message.str();
}

session_limit_exceeded::session_limit_exceeded()
{
    m_message = "Server session limit exceeded.";
}

invalid_reference_offset::invalid_reference_offset(
    gaia::common::gaia_type_t type, gaia::common::reference_offset_t offset)
{
    std::stringstream message;
    message << "Gaia type '" << type << "' has no relationship for the offset '" << offset << "'.";
    m_message = message.str();
}

invalid_relationship_type::invalid_relationship_type(
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

single_cardinality_violation::single_cardinality_violation(gaia::common::gaia_type_t type, gaia::common::reference_offset_t offset)
{
    std::stringstream message;
    message
        << "Gaia type '" << type << "' has single cardinality for the relationship with offset '" << offset
        << "', but multiple children are being added.";
    m_message = message.str();
}

child_already_referenced::child_already_referenced(gaia::common::gaia_type_t child_type, gaia::common::reference_offset_t offset)
{
    std::stringstream message;
    message
        << "Gaia type '" << child_type
        << "' already has a reference for the relationship with offset '" << offset << "'.";
    m_message = message.str();
}

invalid_child::invalid_child(
    gaia::common::gaia_type_t child_type,
    gaia::common::gaia_id_t child_id,
    gaia::common::gaia_type_t parent_type,
    gaia::common::gaia_id_t parent_id)
{
    std::stringstream message;
    message
        << "Impossible to remove child with id '" << child_id
        << "' and type '" << child_type
        << "' from parent with id '" << parent_id
        << "' and type '" << parent_type << "'. The child has a different parent.";
    m_message = message.str();
}

namespace index
{

// This constructor should only be used on client side,
// to re-throw the exception indicated by the server.
unique_constraint_violation::unique_constraint_violation(const char* error_message)
{
    m_message = error_message;
}

// A violation could be triggered by conflict with a non-committed transaction.
// If that transaction fails to commit, its record will not exist.
// This is why no record id is being provided in the message: because it may
// not correspond to any valid record at the time that the error is investigated.
unique_constraint_violation::unique_constraint_violation(const char* error_table_name, const char* error_index_name)
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
