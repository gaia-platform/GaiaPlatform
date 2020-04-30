/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once
#include "storage_engine.hpp"

namespace gaia 
{
namespace rules
{

// This class is used internally to log events in the event table.  We don't
// want to cause any events to be fired in response to an internal transaction
// so use the low-level storage engine api (gaia_mem_base) instead of
// (gaia::db).  Because the storage engine does not currently support
// nested transactions, we check to see whether we are currently in a transaction
// before starting a new one.  This transaction will automatically rollback if
// it goes out of scope and commit is not called.  Correct usage looks like:
class auto_tx_t
{
public:
    auto_tx_t()
    {
        if (!gaia::db::gaia_mem_base::is_tx_active())
        {
            gaia::db::gaia_mem_base::tx_begin();
            end_tx = true;
        }
    }

    void commit()
    {
        if (end_tx)
        {
            gaia::db::gaia_mem_base::tx_commit();
            end_tx = false;
        }
    }

    ~auto_tx_t()
    {
        // If commit() is not called then this transation will
        // automatically rollback the transaction it started.
        if (end_tx)
        {
            gaia::db::gaia_mem_base::tx_rollback();
        }
    }

private:
    bool end_tx = false;
};

} // namespace rules
} // namespace gaia
