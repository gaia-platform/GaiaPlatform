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
// before starting a new one.
class event_tx_t
{
public:
    event_tx_t()
    {
        if (!gaia::db::gaia_mem_base::is_tx_active())
        {
            gaia::db::gaia_mem_base::tx_begin();
            do_commit = true;
        }
    }

    ~event_tx_t()
    {
        if (do_commit)
        {
            gaia::db::gaia_mem_base::tx_commit();
        }
    }

private:
    bool do_commit = false;
};

} // namespace rules
} // namespace gaia
