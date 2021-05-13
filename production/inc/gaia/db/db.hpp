/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <sstream>

#include "gaia/common.hpp"
#include "gaia/exception.hpp"

namespace gaia
{
/**
 * \addtogroup Gaia
 * @{
 */
namespace db
{
/**
 * \addtogroup Db
 * @{
 */

/** \brief A session already exists on this thread.
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

/** \brief No session exists on this thread.
 *
 *  A transaction can only be opened from a thread with an active session.
 */
class no_active_session : public common::gaia_exception
{
public:
    no_active_session()
    {
        m_message = "Open a session before performing data access.";
    }
};

/** \brief A transaction is already in progress in this session.
 *
 *  Only one transaction at a time can exist within a session.
 */
class transaction_in_progress : public common::gaia_exception
{
public:
    transaction_in_progress()
    {
        m_message = "Commit or roll back the current transaction before opening a new transaction.";
    }
};

/** \brief No transaction has been opened in this session.
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

/** \brief The transaction conflicts with another transaction.
 *
 *  If two transactions modify the same data at the same time, one of them must abort.
 */
class transaction_update_conflict : public common::gaia_exception
{
public:
    transaction_update_conflict()
    {
        m_message = "Transaction was aborted due to a serialization error.";
    }
};

/** \brief The transaction attempted to update too many objects.
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

/** \brief The transaction attempted to create an object with an existing ID.
 *
 *  A transaction must create a new object using an ID that has not already been used for another object.
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

/** \brief The transaction tried to create more objects than fit into memory.
 *
 *  The memory used to store objects cannot exceed the configured physical memory limit.
 */
class oom : public common::gaia_exception
{
public:
    oom()
    {
        m_message = "Out of memory.";
    }
};

/** \brief The transaction tried to create more objects than are permitted in the system.
 *
 *  The system cannot contain more than 2^32 objects at one time.
 */
class system_object_limit_exceeded : public common::gaia_exception
{
public:
    system_object_limit_exceeded()
    {
        m_message = "System object limit exceeded.";
    }
};

/** \brief The transaction referenced an object ID that does not exist.
 *
 *  An object can only reference existing objects.
 */
class invalid_object_id : public common::gaia_exception
{
public:
    explicit invalid_object_id(common::gaia_id_t id)
    {
        std::stringstream strs;
        strs << "Cannot find a node with ID '" << id << "'.";
        m_message = strs.str();
    }
};

/** \brief The transaction attempted to delete an object that is referenced by another object.
 *
 *  Objects that are still referenced by existing objects cannot be deleted.
 */
class object_still_referenced : public common::gaia_exception
{
public:
    object_still_referenced(common::gaia_id_t id, common::gaia_type_t object_type)
    {
        std::stringstream msg;
        msg
            << "Cannot delete object '" << id << "', type '" << object_type
            << "', because it is still referenced by another object.";
        m_message = msg.str();
    }
};

/** \brief The transaction attempted to create or update an object that is too large.
 *
 *  An object cannot be larger than 64KB.
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

/** \brief The transaction attempted to create an object with an unknown type.
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
 * \brief Returns true if a transaction is active in this session.
 *
 * \return true if a transaction has been opened in this session, false otherwise.
 */
bool is_transaction_active();

/**
 * \brief Opens a new database session.
 *
 * Opening a session creates a connection to the database server and allocates session-owned resources on both the client and server.
 */
void begin_session();

/**
 * \brief Closes the current database session.
 *
 * Closing a session terminates the connection to the database server and releases session-owned resources on both the client and server.
 */
void end_session();

/**
 * \brief Opens a new database transaction.
 *
 * Opening a transaction creates a snapshot of the database for this session. Objects can only be created, updated, or deleted from within a transaction.
 */
void begin_transaction();

/**
 * \brief Rolls back the current transaction.
 *
 * No changes made by this transaction will be visible to any other transactions.
 */
void rollback_transaction();

/**
 * \brief Submits the current transaction to the server for validation.
 *
 * After the transaction is submitted, it may either commit or abort. A committed transaction's changes will be visible to all future transactions. An aborted transaction's changes will not be visible to any future transactions.
 *
 * \exception gaia::db::transaction_update_conflict transaction conflicts with another transaction.
 */
void commit_transaction();

/*@}*/
} // namespace db
/*@}*/
} // namespace gaia
