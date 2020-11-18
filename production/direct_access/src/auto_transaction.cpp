/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "gaia/auto_transaction.hpp"

namespace gaia
{
namespace direct_access
{

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
    // Someone could have ended the current transaction with an explicit
    // call to gaia::db::commit_transaction or
    // gaia::db::rollback_transaction. Ensure we check this case so that
    // we don't cause an exception here in the destructor.
    if (gaia::db::is_transaction_active())
    {
        gaia::db::rollback_transaction();
    }
}

} // namespace direct_access
} // namespace gaia
