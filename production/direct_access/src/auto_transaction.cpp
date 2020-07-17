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
    gaia::db::begin_transaction();
}

void auto_transaction_t::commit()
{
    gaia::db::commit_transaction();
    if (m_auto_begin)
    {
        gaia::db::begin_transaction();
    }
}

auto_transaction_t::~auto_transaction_t()
{
    if (gaia::db::gaia_mem_base::is_tx_active())
    {
        gaia::db::rollback_transaction();
    }
}

} // direct_access
} // gaia
