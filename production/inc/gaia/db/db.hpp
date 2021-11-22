/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <sstream>

#include "gaia/common.hpp"

// Export all symbols declared in this file.
#pragma GCC visibility push(default)

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

/**
 * \brief Returns true if a session is open in this thread.
 *
 * \return true if a session has been opened in this thread, false otherwise.
 */
bool is_session_open();

/**
 * \brief Returns true if a transaction is open in this session.
 *
 * \return true if a transaction has been opened in this session, false otherwise.
 */
bool is_transaction_open();

/**
 * \brief Opens a new database session.
 *
 * Opening a session creates a connection to the database server and allocates
 * session-owned resources on both the client and the server.
 *
 * \exception gaia::db::session_exists a session is already open in this thread.
 */
void begin_session();

/**
 * \brief Closes the current database session.
 *
 * Closing a session terminates the connection to the database server and
 * releases session-owned resources on both the client and the server.
 *
 * \exception gaia::db::no_open_session no session is open in this thread.
 * \exception gaia::db::transaction_in_progress call commit_transaction() or rollback_transaction() before closing this session.
 */
void end_session();

/**
 * \brief Opens a new database transaction.
 *
 * Gaia supports one open transaction per session.
 * Opening a transaction creates a snapshot of the database for this session.
 * Objects can only be created, updated, or deleted from within a transaction.
 * To terminate the transaction and commit its changes, call commit_transaction().
 * To terminate the transaction without committing its changes, call rollback_transaction().
 *
 * \exception gaia::db::no_open_session open a session before opening a transaction.
 * \exception gaia::db::transaction_in_progress a transaction is already open in this session.
 */
void begin_transaction();

/**
 * \brief Terminates the current transaction in this session.
 *
 * No changes made by this transaction will be visible to any other transactions.
 *
 * \exception gaia::db::no_open_transaction no transaction is open in this session.
 */
void rollback_transaction();

/**
 * \brief Commits the current transaction's changes.
 *
 * The transaction is submitted to the server for validation.
 * If the server doesn't validate the transaction, it aborts the transaction.
 *
 * \exception gaia::db::no_open_transaction no transaction is open in this session.
 * \exception gaia::db::transaction_update_conflict transaction conflicts with another transaction.
 */
void commit_transaction();

/*@}*/
} // namespace db
/*@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
