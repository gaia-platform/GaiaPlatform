/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <map>

#include "gaia/db/db.hpp"

namespace gaia
{

/**
 * \addtogroup Gaia
 * @{
 */

namespace direct_access
{

/**
 * \addtogroup Direct
 * @{
 *
 * Provides a RAII object for exception safe handling of transactions.
 *
 */

/**
 * The auto transaction object begins a transaction on construction and
 * does a rollback on destruction.  The user must explicitly call commit().
 *
 * If an exception occurs during commit, it will prevent the start of
 * another transaction when auto_begin was set; in such situations,
 * begin() can be called to start a new transaction directly.
 */
class auto_transaction_t
{
public:
    static const bool no_auto_begin = false;

public:
    auto_transaction_t()
        : auto_transaction_t(true)
    {
    }
    auto_transaction_t(bool auto_begin);
    ~auto_transaction_t();

    void begin();
    void commit();

private:
    bool m_auto_begin;
};

/*@}*/
} // namespace direct_access
/*@}*/
} // namespace gaia
