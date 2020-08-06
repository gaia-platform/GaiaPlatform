/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "gaia_db.hpp"

namespace gaia
{
namespace rules
{

// This class is used internally to log events in the event table and verify
// rule subscriptions with the catalog.
class auto_tx_t
{
public:
    auto_tx_t()
    {
        if (!gaia::db::is_transaction_active())
        {
            gaia::db::begin_transaction();
            m_started = true;
        }
    }

    void commit()
    {
        if (m_started)
        {
            gaia::db::commit_transaction();
            m_started = false;
        }
    }

    ~auto_tx_t()
    {
        // If commit() is not called then this transation will
        // automatically rollback the transaction it started.
        if (m_started)
        {
            gaia::db::rollback_transaction();
        }
    }

private:
    bool m_started = false;
};

} // namespace rules
} // namespace gaia
