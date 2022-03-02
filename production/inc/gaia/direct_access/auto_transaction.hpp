/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <map>

#include "gaia/db/db.hpp"

// Export all symbols declared in this file.
#pragma GCC visibility push(default)

namespace gaia
{
/**
 * @addtogroup gaia
 * @{
 */
namespace direct_access
{
/**
 * @addtogroup direct_access
 * @{
 */

/**
 * @brief Provides a RAII object for exception safe handling of transactions.
 *
 * The auto transaction object begins a transaction on construction and
 * does a rollback on destruction.  The user must explicitly call commit().
 *
 * If an exception occurs during commit, it will prevent the start of
 * another transaction when auto_restart was set; in such situations,
 * begin() can be called to start a new transaction directly.
 */
class auto_transaction_t
{
public:
    static const bool no_auto_restart = false;

public:
    auto_transaction_t()
        : auto_transaction_t(true)
    {
    }
    auto_transaction_t(bool auto_restart);
    ~auto_transaction_t();

    void begin();
    void commit();

private:
    bool m_auto_restart;
};

/**@}*/
} // namespace direct_access
/**@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
