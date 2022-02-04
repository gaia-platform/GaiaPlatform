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
class forbidden_system_db_operation : public common::gaia_exception
{
};

/**
 * Thrown when a database already exists with the specified name.
 */
class db_already_exists : public common::gaia_exception
{
};

/**
 * Thrown when a specified database does not exist.
 */
class db_does_not_exist : public common::gaia_exception
{
};

/**
 * Thrown when a table already exists with the specified name.
 */
class table_already_exists : public common::gaia_exception
{
};

/**
 * Thrown when a specified table does not exist.
 */
class table_does_not_exist : public common::gaia_exception
{
};

/**
 * Thrown when a field is specified more than once.
 */
class duplicate_field : public common::gaia_exception
{
};

/**
 * Thrown when a specified field does not exist.
 */
class field_does_not_exist : public common::gaia_exception
{
};

/**
 * Thrown when the maximum number of references has been reached for a type.
 */
class max_reference_count_reached : public common::gaia_exception
{
};

/**
 * Thrown when a database operation would violate referential integrity.
 */
class referential_integrity_violation : public common::gaia_exception
{
};

/**
 * Thrown when a relationship already exists with the specified name.
 */
class relationship_already_exists : public common::gaia_exception
{
};

/**
 * Thrown when a relationship does not exist.
 */
class relationship_does_not_exist : public common::gaia_exception
{
};

/**
 * Thrown when creating a relationship between tables from different databases.
 */
class no_cross_db_relationship : public common::gaia_exception
{
};

/**
 * Thrown when the tables specified in the relationship definition do not match.
 */
class relationship_tables_do_not_match : public common::gaia_exception
{
};

/**
 * Thrown when trying to create a many-to-many relationship.
 */
class many_to_many_not_supported : public common::gaia_exception
{
};

/**
 * Thrown when creating an index that already exists.
 */
class index_already_exists : public common::gaia_exception
{
};

/**
 * Thrown when the index of the given name does not exists.
 */
class index_does_not_exist : public common::gaia_exception
{
};

/**
 * Thrown when the fields used in a relationship are invalid.
 */
class invalid_relationship_field : public common::gaia_exception
{
};

/**
 * Thrown when the `references` definition can match multiple `references`
 * definitions elsewhere.
 */
class ambiguous_reference_definition : public common::gaia_exception
{
};

/**
 * Thrown when the `references` definition does not have a matching definition.
 */
class orphaned_reference_definition : public common::gaia_exception
{
};

/**
 * Thrown when the create list is invalid.
 */
class invalid_create_list : public common::gaia_exception
{
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
 * Thrown when an invalid Gaia configuration setting is detected.
 */
class configuration_error : public gaia_exception
{
};

/**
 * Thrown when accessing an optional that has no value.
 */
class optional_value_not_found : public common::gaia_exception
{
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
 * Only one session at a time can exist on a thread.
 */
class session_exists : public common::gaia_exception
{
};

/**
 * \brief No session exists on this thread.
 *
 * A transaction can only be opened from a thread with an open session.
 */
class no_open_session : public common::gaia_exception
{
};

/**
 * \brief A transaction is already in progress in this session.
 *
 * Only one transaction at a time can exist within a session.
 */
class transaction_in_progress : public common::gaia_exception
{
};

/**
 * \brief No transaction is open in this session.
 *
 * Data can only be accessed from an open transaction.
 */
class no_open_transaction : public common::gaia_exception
{
};

/**
 * \brief The transaction conflicts with another transaction.
 *
 * If two transactions modify the same data at the same time, one of them must abort.
 */
class transaction_update_conflict : public common::gaia_exception
{
};

/**
 * \brief The transaction attempted to update too many objects.
 *
 * A transaction can create, update, or delete at most 2^20 objects.
 */
class transaction_object_limit_exceeded : public common::gaia_exception
{
};

/**
 * \brief The transaction attempted to create an object with an existing ID.
 *
 * A transaction must create a new object using an ID that has not been assigned to another object.
 */
class duplicate_object_id : public common::gaia_exception
{
};

/**
 * \brief The transaction tried to create more objects than fit into memory.
 *
 * The memory used to store objects cannot exceed the configured physical memory limit.
 */
class out_of_memory : public common::gaia_exception
{
};

/**
 * \brief The transaction exceeded its memory limit.
 *
 * A transaction can use at most 44MB of memory.
 */
class transaction_memory_limit_exceeded : public common::gaia_exception
{
};


/**
 * \brief The transaction tried to create more objects than are permitted in the system.
 *
 * The system cannot contain more than 2^32 objects.
 */
class system_object_limit_exceeded : public common::gaia_exception
{
};

/**
 * \brief The transaction referenced an object ID that does not exist.
 *
 * An object can only reference existing objects.
 */
class invalid_object_id : public common::gaia_exception
{
};

/**
 * \brief The transaction attempted to delete an object that is referenced by another object.
 *
 * Objects that are still referenced by existing objects cannot be deleted.
 */
class object_still_referenced : public common::gaia_exception
{
};

/**
 * \brief The transaction attempted to create or update an object that is too large.
 *
 * An object cannot be larger than 64 KB.
 */
class object_too_large : public common::gaia_exception
{
};

/**
 * \brief The transaction referenced an object type that does not exist
 * or that does not match the expected type.
 *
 * An object's type must exist in the catalog or must match the expected object type.
 */
class invalid_object_type : public common::gaia_exception
{
};

/**
 * Thrown when the Gaia server session limit has been exceeded.
 */
class session_limit_exceeded : public common::gaia_exception
{
};

/**
 * Thrown when adding a reference to an offset that does not exist. It could surface
 * when a relationship is deleted at runtime, but the DAC classes are not up to
 * date with it.
 */
class invalid_reference_offset : public common::gaia_exception
{
};

/**
 * Thrown when adding a reference to an offset that exists but the type of the object
 * that is being added is of the wrong type according to the relationship definition.
 * This can happen when the relationships are modified at runtime and the DAC classes
 * are not up to date with it.
 */
class invalid_relationship_type : public common::gaia_exception
{
};

/**
 * Thrown when adding more than one child to a one-to-one relationship.
 * This can happen when the relationships are modified at runtime and the DAC classes
 * are not up to date with it.
 */
class single_cardinality_violation : public common::gaia_exception
{
};

/**
 * Thrown when adding a child to a relationship that already contains it.
 */
class child_already_referenced : public common::gaia_exception
{
};

/**
 * Thrown when the type of a child reference does not match the type of the parent.
 */
class invalid_child_reference : public common::gaia_exception
{
};

/**
 * Thrown when failing to allocate more memory.
 */
class memory_allocation_error : public common::gaia_exception
{
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
 * Thrown to indicate the violation of a UNIQUE constraint.
 *
 * Extends pre_commit_validation_failure to indicate that this exception
 * is expected to occur during the pre-commit processing of a transaction.
 */
class unique_constraint_violation : public pre_commit_validation_failure
{
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

/**
 * Thrown when an object's internal state is not as expected.
 */
class invalid_object_state : public common::gaia_exception
{
};

/*@}*/
} // namespace direct_access

/*@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
