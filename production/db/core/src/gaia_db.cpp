/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia/db/db.hpp"

#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/db/gaia_db_internal.hpp"

#include "db_client.hpp"

bool gaia::db::is_transaction_active()
{
    return gaia::db::client::is_transaction_active();
}

void gaia::db::begin_session()
{
    gaia::db::client::begin_session();
}

void gaia::db::end_session()
{
    gaia::db::client::end_session();
}

void gaia::db::begin_transaction()
{
    gaia::db::client::begin_transaction();
}

void gaia::db::rollback_transaction()
{
    gaia::db::client::rollback_transaction();
}

void gaia::db::commit_transaction()
{
    gaia::db::client::commit_transaction();
}

void gaia::db::set_commit_trigger(gaia::db::triggers::commit_trigger_fn trigger_fn)
{
    gaia::db::client::set_commit_trigger(trigger_fn);
}

void gaia::db::clear_shared_memory()
{
    gaia::db::client::clear_shared_memory();
}

gaia::db::gaia_txn_id_t gaia::db::get_txn_id()
{
    return gaia::db::client::get_txn_id();
}

// Implements Murmur3 64-bit finalizer
// (https://github.com/aappleby/smhasher/wiki/MurmurHash3).
// This will map 0 to 0 and acts as a random permutation on all other integer
// values.
static inline uint64_t mix_uint64(uint64_t x)
{
    x ^= x >> 33ULL;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33ULL;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33ULL;
    return x;
}

// Publicly expose begin timestamp of current txn in obfuscated form,
// to avoid clients depending on sequential txn ids.
// Returns uint64_t since gaia_txn_id_t isn't a public type.
uint64_t gaia::db::get_transaction_id()
{
    if (!gaia::db::client::is_transaction_active())
    {
        return 0;
    }
    return mix_uint64(gaia::db::client::get_txn_id());
}
