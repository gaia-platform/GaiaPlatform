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

namespace common
{
/**
 * \addtogroup common
 * @{
 */

/**
 * An exception class used for unexpected library API failures that return error codes.
 */
class api_error : public gaia_exception
{
public:
    api_error(const char* api_name, int error_code);
};

/**
 * An exception class used to indicate invalid Gaia configuration settings.
 */
class configuration_error : public gaia_exception
{
public:
    explicit configuration_error(const char* key, int value);
    explicit configuration_error(const char* filename);
};

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
    session_exists()
    {
        m_message = "Close the current session before opening a new one.";
    }
};

/**
 * \brief No session exists on this thread.
 *
 *  A transaction can only be opened from a thread with an open session.
 */
class no_open_session : public common::gaia_exception
{
public:
    no_open_session()
    {
        m_message = "Open a session before performing data access.";
    }
};

/**
 * \brief A transaction is already in progress in this session.
 *
 *  Only one transaction at a time can exist within a session.
 */
class transaction_in_progress : public common::gaia_exception
{
public:
    transaction_in_progress()
    {
        m_message = "Commit or rollback the current transaction before opening a new transaction.";
    }
};

/**
 * \brief No transaction is open in this session.
 *
 *  Data can only be accessed from an open transaction.
 */
class no_open_transaction : public common::gaia_exception
{
public:
    no_open_transaction()
    {
        m_message = "Open a transaction before performing data access.";
    }
};

/**
 * \brief The transaction conflicts with another transaction.
 *
 *  If two transactions modify the same data at the same time, one of them must abort.
 */
class transaction_update_conflict : public common::gaia_exception
{
public:
    transaction_update_conflict()
    {
        m_message = "Transaction was aborted due to a conflict with another transaction.";
    }
};

/**
 * \brief The transaction attempted to update too many objects.
 *
 *  A transaction can create, update, or delete at most 2^20 objects.
 */
class transaction_object_limit_exceeded : public common::gaia_exception
{
public:
    transaction_object_limit_exceeded()
    {
        m_message = "Transaction attempted to update too many objects.";
    }
};

/**
 * \brief The transaction attempted to create an object with an existing ID.
 *
 *  A transaction must create a new object using an ID that has not been assigned to another object.
 */
class duplicate_id : public common::gaia_exception
{
public:
    explicit duplicate_id(common::gaia_id_t id)
    {
        std::stringstream strs;
        strs << "An object with the same ID '" << id << "' already exists.";
        m_message = strs.str();
    }
};

/**
 * \brief The transaction tried to create more objects than fit into memory.
 *
 *  The memory used to store objects cannot exceed the configured physical memory limit.
 */
class out_of_memory : public common::gaia_exception
{
public:
    out_of_memory()
    {
        m_message = "Out of memory.";
    }
};

/**
 * \brief The transaction tried to create more objects than are permitted in the system.
 *
 *  The system cannot contain more than 2^32 objects.
 */
class system_object_limit_exceeded : public common::gaia_exception
{
public:
    system_object_limit_exceeded()
    {
        m_message = "System object limit exceeded.";
    }
};

/**
 * \brief The transaction referenced an object ID that does not exist.
 *
 *  An object can only reference existing objects.
 */
class invalid_object_id : public common::gaia_exception
{
public:
    explicit invalid_object_id(common::gaia_id_t id)
    {
        std::stringstream strs;
        strs << "Cannot find an object with ID '" << id << "'.";
        m_message = strs.str();
    }
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
        common::gaia_id_t other_id, common::gaia_type_t other_type)
    {
        std::stringstream msg;
        msg
            << "Cannot delete object with ID '" << id << "', type '" << object_type
            << "', because it is still referenced by another object with ID '"
            << other_id << "', type '" << other_type << "'";
        m_message = msg.str();
    }
};

/**
 * \brief The transaction attempted to create or update an object that is too large.
 *
 *  An object cannot be larger than 64 KB.
 */
class object_too_large : public common::gaia_exception
{
public:
    object_too_large(size_t total_len, uint16_t max_len)
    {
        std::stringstream msg;
        msg << "Object size " << total_len << " exceeds maximum size " << max_len << ".";
        m_message = msg.str();
    }
};

/**
 * \brief The transaction attempted to create an object with an unknown type.
 *
 *  An object's type must exist in the catalog.
 */
class invalid_type : public common::gaia_exception
{
public:
    explicit invalid_type(common::gaia_type_t type)
    {
        std::stringstream msg;
        msg << "The type '" << type << "' does not exist in the catalog.";
        m_message = msg.str();
    }

    invalid_type(common::gaia_id_t id, common::gaia_type_t type)
    {
        std::stringstream msg;
        msg
            << "Cannot create object with ID '" << id << "' and type '" << type
            << "'. The type does not exist in the catalog.";
        m_message = msg.str();
    }
};

/**
 * An exception class used to indicate that the Gaia server session limit has been exceeded.
 */
class session_limit_exceeded : public common::gaia_exception
{
public:
    session_limit_exceeded()
    {
        m_message = "Server session limit exceeded.";
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
    explicit unique_constraint_violation(const char* error_message)
    {
        m_message = error_message;
    }

    // A violation could be triggered by conflict with a non-committed transaction.
    // If that transaction fails to commit, its record will not exist.
    // This is why no record id is being provided in the message: because it may
    // not correspond to any valid record at the time that the error is investigated.
    unique_constraint_violation(const char* error_table_name, const char* error_index_name)
    {
        std::stringstream message;
        message
            << c_error_description
            << " Cannot insert a duplicate key in table: '" << error_table_name << "', because of the unique constraint of "
            << " index: '" << error_index_name << "'.";
        m_message = message.str();
    }
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
