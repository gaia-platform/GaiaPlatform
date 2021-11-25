/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstring>

#include <sstream>

#include "gaia/common.hpp"
#include "gaia/exception.hpp"

// Export all symbols declared in this file.
#pragma GCC visibility push(default)

namespace gaia
{
/**
 * \addtogroup gaia
 * @{
 */

namespace catalog
{
/**
 * \addtogroup catalog
 * @{
 */

/**
 * Thrown when attempting to modify a system database.
 */
class forbidden_system_db_operation : public gaia::common::gaia_exception
{
public:
    explicit forbidden_system_db_operation(const std::string& name);
};

/**
 * Thrown when a database already exists with the specified name.
 */
class db_already_exists : public gaia::common::gaia_exception
{
public:
    explicit db_already_exists(const std::string& name);
};

/**
 * Thrown when a specified database does not exist.
 */
class db_not_exists : public gaia::common::gaia_exception
{
public:
    explicit db_not_exists(const std::string& name);
};

/**
 * Thrown when a table already exists with the specified name.
 */
class table_already_exists : public gaia::common::gaia_exception
{
public:
    explicit table_already_exists(const std::string& name);
};

/**
 * Thrown when a specified table does not exist.
 */
class table_not_exists : public gaia::common::gaia_exception
{
public:
    explicit table_not_exists(const std::string& name);
};

/**
 * Thrown when a field is specified more than once.
 */
class duplicate_field : public gaia::common::gaia_exception
{
public:
    explicit duplicate_field(const std::string& name);
};

/**
 * Thrown when a specified field does not exist.
 */
class field_not_exists : public gaia::common::gaia_exception
{
public:
    explicit field_not_exists(const std::string& name);
};

/**
 * Thrown when the maximum number of references has been reached for a type.
 */
class max_reference_count_reached : public gaia::common::gaia_exception
{
public:
    explicit max_reference_count_reached();
};

class referential_integrity_violation : public gaia::common::gaia_exception
{
public:
    explicit referential_integrity_violation(const std::string& message);

    static referential_integrity_violation drop_referenced_table(
        const std::string& referenced_table,
        const std::string& referencing_table);
};

/**
 * Thrown when a relationship already exists with the specified name.
 */
class relationship_already_exists : public gaia::common::gaia_exception
{
public:
    explicit relationship_already_exists(const std::string& name);
};

/**
 * Thrown when a relationship does not exist.
 */
class relationship_not_exists : public gaia::common::gaia_exception
{
public:
    explicit relationship_not_exists(const std::string& name);
};

/**
 * Thrown when creating a relationship between tables from different databases.
 */
class no_cross_db_relationship : public gaia::common::gaia_exception
{
public:
    explicit no_cross_db_relationship(const std::string& name);
};

/**
 * Thrown when the tables specified in the relationship definition do not match.
 */
class tables_not_match : public gaia::common::gaia_exception
{
public:
    explicit tables_not_match(
        const std::string& relationship,
        const std::string& name1,
        const std::string& name2);
};

/**
 * Thrown when trying to create a many-to-many relationship.
 */
class many_to_many_not_supported : public gaia::common::gaia_exception
{
public:
    explicit many_to_many_not_supported(const std::string& relationship);

    explicit many_to_many_not_supported(const std::string& table1, const std::string& table2);
};

/**
 * Thrown when creating an index that already exists.
 */
class index_already_exists : public gaia::common::gaia_exception
{
public:
    explicit index_already_exists(const std::string& name);
};

/**
 * Thrown when the index of the given name does not exists.
 */
class index_not_exists : public gaia::common::gaia_exception
{
public:
    explicit index_not_exists(const std::string& name);
};

/**
 * Thrown when the field map is invalid.
 */
class invalid_field_map : public gaia::common::gaia_exception
{
public:
    explicit invalid_field_map(const std::string& message);
};

/**
 * Thrown when the `references` definition can match multiple `references`
 * definitions elsewhere.
 */
class ambiguous_reference_definition : public gaia::common::gaia_exception
{
public:
    explicit ambiguous_reference_definition(const std::string& table, const std::string& ref_name);
};

/**
 * Thrown when the `references` definition does not have a matching definition.
 */
class orphaned_reference_definition : public gaia::common::gaia_exception
{
public:
    explicit orphaned_reference_definition(const std::string& table, const std::string& ref_name);
};

/**
 * Thrown when the create list is invalid.
 */
class invalid_create_list : public gaia::common::gaia_exception
{
public:
    explicit invalid_create_list(const std::string& message);
};

/**
 * Thrown when dropping a table that still has (user) data.
 */
class cannot_drop_table_with_data : public gaia::common::gaia_exception
{
public:
    explicit cannot_drop_table_with_data(const std::string& name);
};

/*@}*/
} // namespace catalog

namespace common
{
/**
 * \addtogroup common
 * @{
 */

/**
 * An exception class used to indicate invalid Gaia configuration settings.
 */
class configuration_error : public gaia_exception
{
public:
    explicit configuration_error(const char* key, int value);
    explicit configuration_error(const char* filename);
};

namespace logging
{
/**
 * \addtogroup logging
 * @{
 */

/**
 * Thrown on logging system failures.
 */
class logger_exception : public gaia_exception
{
public:
    explicit logger_exception(const std::string& message)
    {
        m_message = message;
    }
};

/*@}*/
} // namespace logging

/*@}*/
} // namespace common

namespace db
{
/**
 * \addtogroup db
 * @{
 */

/**
 * \brief A session already exists on this thread.
 *
 *  Only one session at a time can exist on a thread.
 */
class session_exists : public common::gaia_exception
{
public:
    session_exists();
};

/**
 * \brief No session exists on this thread.
 *
 *  A transaction can only be opened from a thread with an open session.
 */
class no_open_session : public common::gaia_exception
{
public:
    no_open_session();
};

/**
 * \brief A transaction is already in progress in this session.
 *
 *  Only one transaction at a time can exist within a session.
 */
class transaction_in_progress : public common::gaia_exception
{
public:
    transaction_in_progress();
};

/**
 * \brief No transaction is open in this session.
 *
 *  Data can only be accessed from an open transaction.
 */
class no_open_transaction : public common::gaia_exception
{
public:
    no_open_transaction();
};

/**
 * \brief The transaction conflicts with another transaction.
 *
 *  If two transactions modify the same data at the same time, one of them must abort.
 */
class transaction_update_conflict : public common::gaia_exception
{
public:
    transaction_update_conflict();
};

/**
 * \brief The transaction attempted to update too many objects.
 *
 *  A transaction can create, update, or delete at most 2^20 objects.
 */
class transaction_object_limit_exceeded : public common::gaia_exception
{
public:
    transaction_object_limit_exceeded();
};

/**
 * \brief The transaction attempted to create an object with an existing ID.
 *
 *  A transaction must create a new object using an ID that has not been assigned to another object.
 */
class duplicate_id : public common::gaia_exception
{
public:
    explicit duplicate_id(common::gaia_id_t id);
};

/**
 * \brief The transaction tried to create more objects than fit into memory.
 *
 *  The memory used to store objects cannot exceed the configured physical memory limit.
 */
class out_of_memory : public common::gaia_exception
{
public:
    out_of_memory();
};

/**
 * \brief The transaction tried to create more objects than are permitted in the system.
 *
 *  The system cannot contain more than 2^32 objects.
 */
class system_object_limit_exceeded : public common::gaia_exception
{
public:
    system_object_limit_exceeded();
};

/**
 * \brief The transaction referenced an object ID that does not exist.
 *
 *  An object can only reference existing objects.
 */
class invalid_object_id : public common::gaia_exception
{
public:
    explicit invalid_object_id(common::gaia_id_t id);
};

/**
 * \brief The transaction attempted to delete an object that is referenced by another object.
 *
 *  Objects that are still referenced by existing objects cannot be deleted.
 */
class object_still_referenced : public common::gaia_exception
{
public:
    object_still_referenced(
        common::gaia_id_t id, common::gaia_type_t object_type,
        common::gaia_id_t other_id, common::gaia_type_t other_type);
};

/**
 * \brief The transaction attempted to create or update an object that is too large.
 *
 *  An object cannot be larger than 64 KB.
 */
class object_too_large : public common::gaia_exception
{
public:
    object_too_large(size_t total_len, uint16_t max_len);
};

/**
 * \brief The transaction attempted to create an object with an unknown type.
 *
 *  An object's type must exist in the catalog.
 */
class invalid_type : public common::gaia_exception
{
public:
    explicit invalid_type(common::gaia_type_t type);

    invalid_type(common::gaia_id_t id, common::gaia_type_t type);
};

/**
 * An exception class used to indicate that the Gaia server session limit has been exceeded.
 */
class session_limit_exceeded : public common::gaia_exception
{
public:
    session_limit_exceeded();
};

/**
 * Thrown when adding a reference to an offset that does not exist. It could surface
 * when a relationship is deleted at runtime, but the DAC classes are not up to
 * date with it.
 */
class invalid_reference_offset : public gaia::common::gaia_exception
{
public:
    invalid_reference_offset(gaia::common::gaia_type_t type, gaia::common::reference_offset_t offset);
};

/**
 * Thrown when adding a reference to an offset that exists but the type of the object
 * that is being added is of the wrong type according to the relationship definition.
 * This can happen when the relationships are modified at runtime and the DAC classes
 * are not up to date with it.
 */
class invalid_relationship_type : public gaia::common::gaia_exception
{
public:
    invalid_relationship_type(
        gaia::common::reference_offset_t offset,
        gaia::common::gaia_type_t expected_type,
        gaia::common::gaia_type_t found_type);
};

/**
 * Thrown when adding more than one child to a one-to-one relationship.
 * This can happen when the relationships are modified at runtime and the DAC classes
 * are not up to date with it.
 */
class single_cardinality_violation : public gaia::common::gaia_exception
{
public:
    single_cardinality_violation(gaia::common::gaia_type_t type, gaia::common::reference_offset_t offset);
};

/**
 * Thrown when adding a child to a relationship that already contains it.
 */
class child_already_referenced : public gaia::common::gaia_exception
{
public:
    child_already_referenced(gaia::common::gaia_type_t child_type, gaia::common::reference_offset_t offset);
};

class invalid_child : public gaia::common::gaia_exception
{
public:
    invalid_child(
        gaia::common::gaia_type_t child_type,
        gaia::common::gaia_id_t child_id,
        gaia::common::gaia_type_t parent_type,
        gaia::common::gaia_id_t parent_id);
};

/**
 * Thrown when failing to allocate more memory.
 */
class memory_allocation_error : public gaia::common::gaia_exception
{
public:
    explicit memory_allocation_error(const std::string& message)
        : gaia_exception(message)
    {
    }
};

/**
 * A base exception class for any fatal pre-commit validation failure.
 *
 * Any expected pre-commit validation failure exception is expected to extend this class.
 */
class pre_commit_validation_failure : public common::gaia_exception
{
};

namespace index
{
/**
 * \addtogroup index
 * @{
 */

/**
 * An exception class used to indicate the violation of a UNIQUE constraint.
 *
 * Extends pre_commit_validation_failure to indicate that this exception
 * is expected to occur during the pre-commit processing of a transaction.
 */
class unique_constraint_violation : public pre_commit_validation_failure
{
public:
    static constexpr char c_error_description[] = "UNIQUE constraint violation!";

public:
    // This constructor should only be used on client side,
    // to re-throw the exception indicated by the server.
    explicit unique_constraint_violation(const char* error_message);

    // A violation could be triggered by conflict with a non-committed transaction.
    // If that transaction fails to commit, its record will not exist.
    // This is why no record id is being provided in the message: because it may
    // not correspond to any valid record at the time that the error is investigated.
    unique_constraint_violation(const char* error_table_name, const char* error_index_name);
};

/*@}*/
} // namespace index

/*@}*/
} // namespace db

namespace direct_access
{
/**
 * \addtogroup direct_access
 * @{
 */

// Exception when get() argument does not match the class type.
class invalid_object_type : public common::gaia_exception
{
public:
    invalid_object_type(
        common::gaia_id_t id,
        common::gaia_type_t expected_type,
        const char* expected_typename,
        common::gaia_type_t actual_type);
};

// A child's parent pointer must match the parent record we have.
class invalid_member : public common::gaia_exception
{
public:
    invalid_member(
        common::gaia_id_t id,
        common::gaia_type_t parent,
        const char* parent_type,
        common::gaia_type_t child,
        const char* child_name);
};

// When a child refers to a parent, but is not found in that parent's list.
class inconsistent_list : public common::gaia_exception
{
public:
    inconsistent_list(
        common::gaia_id_t id,
        const char* parent_type,
        common::gaia_id_t child,
        const char* child_name);
};

// To connect two objects, a gaia_id() is needed but not available until SE create is called during
// the insert_row().
class invalid_state : public common::gaia_exception
{
public:
    invalid_state(
        common::gaia_id_t parent_id,
        common::gaia_id_t chile_id,
        const char* child_type);
};

// An attempt has been made to insert a member that has already been inserted somewhere.
class already_inserted : public common::gaia_exception
{
public:
    already_inserted(common::gaia_id_t parent, const char* parent_type);
};

/*@}*/
} // namespace direct_access

/*@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
