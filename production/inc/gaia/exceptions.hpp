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

/**
 * @defgroup catalog catalog
 * @ingroup gaia
 * Catalog namespace
 */

/**
 * @defgroup index index
 * @ingroup db
 * Index namespace
 */

namespace gaia
{
/**
 * @addtogroup gaia
 * @{
 */

namespace catalog
{
/**
 * @addtogroup catalog
 * @{
 */

/**
 * @brief Trying to modify a system database.
 */
class forbidden_system_db_operation : public common::gaia_exception
{
};

/**
 * @brief A database already exists with the specified name.
 */
class db_already_exists : public common::gaia_exception
{
};

/**
 * @brief A specified database does not exist.
 */
class db_does_not_exist : public common::gaia_exception
{
};

/**
 * @brief A table already exists with the specified name.
 */
class table_already_exists : public common::gaia_exception
{
};

/**
 * @brief A specified table does not exist.
 */
class table_does_not_exist : public common::gaia_exception
{
};

/**
 * @brief A field is specified more than once.
 */
class duplicate_field : public common::gaia_exception
{
};

/**
 * @brief A specified field does not exist.
 */
class field_does_not_exist : public common::gaia_exception
{
};

/**
 * @brief The maximum number of references has been reached for a type.
 */
class max_reference_count_reached : public common::gaia_exception
{
};

/**
 * @brief A database operation would violate referential integrity.
 */
class referential_integrity_violation : public common::gaia_exception
{
};

/**
 * @brief A relationship already exists with the specified name.
 */
class relationship_already_exists : public common::gaia_exception
{
};

/**
 * @brief A relationship does not exist.
 */
class relationship_does_not_exist : public common::gaia_exception
{
};

/**
 * @brief Trying to create a relationship between tables from different databases.
 */
class no_cross_db_relationship : public common::gaia_exception
{
};

/**
 * @brief The tables specified in the relationship definition do not match.
 */
class relationship_tables_do_not_match : public common::gaia_exception
{
};

/**
 * @brief Trying to create a many-to-many relationship.
 */
class many_to_many_not_supported : public common::gaia_exception
{
};

/**
 * @brief Trying to create an index that already exists.
 */
class index_already_exists : public common::gaia_exception
{
};

/**
 * @brief The index of the given name does not exists.
 */
class index_does_not_exist : public common::gaia_exception
{
};

/**
 * @brief The fields used in a relationship are invalid.
 */
class invalid_relationship_field : public common::gaia_exception
{
};

/**
 * @brief The `references` definition can match multiple `references`
 * definitions elsewhere.
 */
class ambiguous_reference_definition : public common::gaia_exception
{
};

/**
 * @brief The `references` definition does not have a matching definition.
 */
class orphaned_reference_definition : public common::gaia_exception
{
};

/**
 * @brief The create list is invalid.
 */
class invalid_create_list : public common::gaia_exception
{
};

/**@}*/
} // namespace catalog

namespace common
{
/**
 * @addtogroup common
 * @{
 */

/**
 * @brief An invalid Gaia configuration setting was detected.
 */
class configuration_error : public gaia_exception
{
};

/**
 * @brief Trying to access an optional that has no value.
 */
class optional_value_not_found : public common::gaia_exception
{
};

namespace logging
{
/**
 * @addtogroup logging
 * @{
 */

/**
 * @brief Indicates a logging system failure.
 */
class logger_exception : public gaia_exception
{
};

/**@}*/
} // namespace logging

/**@}*/
} // namespace common

namespace db
{
/**
 * @addtogroup db
 * @{
 */

/**
 * @brief Client failed to connect to the server.
 *
 * Server needs to be running.
 */
class server_connection_failed : public common::gaia_exception
{
};

/**
 * @brief A session already exists on this thread.
 *
 * Only one session at a time can exist on a thread.
 */
class session_exists : public common::gaia_exception
{
};

/**
 * @brief No session exists on this thread.
 *
 * A transaction can only be opened from a thread with an open session.
 */
class no_open_session : public common::gaia_exception
{
};

/**
 * @brief A transaction is already in progress in this session.
 *
 * Only one transaction at a time can exist within a session.
 */
class transaction_in_progress : public common::gaia_exception
{
};

/**
 * @brief No transaction is open in this session.
 *
 * Data can only be accessed from an open transaction.
 */
class no_open_transaction : public common::gaia_exception
{
};

/**
 * @brief The transaction conflicts with another transaction.
 *
 * If two transactions modify the same data at the same time, one of them must abort.
 */
class transaction_update_conflict : public common::gaia_exception
{
};

/**
 * @brief The transaction tried to update too many objects.
 *
 * A transaction can create, update, or delete at most 2^16 objects.
 */
class transaction_object_limit_exceeded : public common::gaia_exception
{
};

/**
 * @brief Unable to allocate a log for this transaction.
 *
 * The system can allocate at most 2^16 transaction logs at one time.
 */
class transaction_log_allocation_failure : public common::gaia_exception
{
};

/**
 * @brief The transaction tried to create an object with an existing ID.
 *
 * A transaction must create a new object using an ID that has not been assigned to another object.
 */
class duplicate_object_id : public common::gaia_exception
{
};

/**
 * @brief The transaction tried to create more objects than fit into memory.
 *
 * The memory used to store objects cannot exceed the configured physical memory limit.
 */
class out_of_memory : public common::gaia_exception
{
};

/**
 * @brief The transaction tried to create more objects than are permitted in the system.
 *
 * The system cannot contain more than 2^32 objects.
 */
class system_object_limit_exceeded : public common::gaia_exception
{
};

/**
 * @brief The transaction referenced an object ID that does not exist.
 *
 * An object can only reference existing objects.
 */
class invalid_object_id : public common::gaia_exception
{
};

/**
 * @brief The transaction tried to delete an object that is referenced by another object.
 *
 * Objects that are still referenced by existing objects cannot be deleted.
 */
class object_still_referenced : public common::gaia_exception
{
};

/**
 * @brief The transaction tried to create or update an object that is too large.
 *
 * An object cannot be larger than 64 KB.
 */
class object_too_large : public common::gaia_exception
{
};

/**
 * @brief The transaction referenced an object type that does not exist
 * or that does not match the expected type.
 *
 * An object's type must exist in the catalog or must match the expected object type.
 */
class invalid_object_type : public common::gaia_exception
{
};

/**
 * @brief The registered type limit has been exceeded.
 */
class type_limit_exceeded : public common::gaia_exception
{
};

/**
 * @brief The Gaia server session limit has been exceeded.
 */
class session_limit_exceeded : public common::gaia_exception
{
};

/**
 * @brief A generic session failure. Check the server output for
 * more details.
 */
class session_failure : public common::gaia_exception
{
};

/**
 * @brief Trying to add a reference to an offset that does not exist.
 *
 * This can happen when a relationship is deleted at runtime,
 * but the DAC classes are not up to date with it.
 */
class invalid_reference_offset : public common::gaia_exception
{
};

/**
 * @brief Trying to add a reference to an offset that exists but the type of the object
 * that is being added is of the wrong type according to the relationship definition.
 *
 * This can happen when the relationships are modified at runtime and the DAC classes
 * are not up to date with it.
 */
class invalid_relationship_type : public common::gaia_exception
{
};

/**
 * @brief Trying to add more than one child to a one-to-one relationship.
 *
 * This can happen when the relationships are modified at runtime and the DAC classes
 * are not up to date with it.
 */
class single_cardinality_violation : public common::gaia_exception
{
};

/**
 * @brief Trying to add a child to a relationship that already contains it.
 */
class child_already_referenced : public common::gaia_exception
{
};

/**
 * @brief The type of a child reference does not match the type of the parent.
 */
class invalid_child_reference : public common::gaia_exception
{
};

/**
 * @brief Failed to allocate more memory.
 */
class memory_allocation_error : public common::gaia_exception
{
};

/**
 * @brief A base exception class for any fatal pre-commit validation failure.
 *
 * Any expected pre-commit validation failure exception is expected to extend this class.
 */
class pre_commit_validation_failure : public common::gaia_exception
{
};

namespace index
{
/**
 * @addtogroup index
 * @{
 */

/**
 * @brief Indicates the violation of a UNIQUE constraint.
 *
 * Extends pre_commit_validation_failure to indicate that this exception
 * is expected to occur during the pre-commit processing of a transaction.
 */
class unique_constraint_violation : public pre_commit_validation_failure
{
};

/**@}*/
} // namespace index

/**@}*/
} // namespace db

namespace direct_access
{
/**
 * @addtogroup direct_access
 * @{
 */

/**
 * @brief An object's internal state is not as expected.
 */
class invalid_object_state : public common::gaia_exception
{
};

/**@}*/
} // namespace direct_access

/**@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
