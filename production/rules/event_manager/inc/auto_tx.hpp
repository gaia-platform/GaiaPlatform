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
// (gaia::db).
class auto_tx_t
{
public:
    auto_tx_t()
    {
        gaia::db::gaia_mem_base::tx_begin();
    }

    void commit()
    {
        gaia::db::gaia_mem_base::tx_commit();
        m_committed = true;
    }

    ~auto_tx_t()
    {
        // If commit() is not called then this transation will
        // automatically rollback the transaction it started.
        if (!m_committed)
        {
            gaia::db::gaia_mem_base::tx_rollback();
        }
    }

private:
    bool m_committed = false;
};

} // namespace rules
} // namespace gaia
