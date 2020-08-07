/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "auto_transaction.hpp"

namespace gaia 
{
namespace direct_access 
{

//
// The auto_transaction_t implementation.
//
auto_transaction_t::auto_transaction_t(bool auto_begin)
{
    m_auto_begin = auto_begin;
    // We currently do not allow nested transactions so don't start one if one
    // is active
    if (!gaia::db::is_transaction_active())
    {
        gaia::db::begin_transaction();
    }
}

void auto_transaction_t::commit()
{
    if (gaia::db::is_transaction_active())
    {
        gaia::db::commit_transaction();
    }

    if (m_auto_begin)
    {
        gaia::db::begin_transaction();
    }
}

auto_transaction_t::~auto_transaction_t()
{    
    if (gaia::db::is_transaction_active())
    {
        gaia::db::rollback_transaction();
    }
}

} // direct_access
} // gaia
