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
    return gaia::db::client_t::is_transaction_active();
}

void gaia::db::begin_session(gaia::db::session_opts_t session_opts)
{
    gaia::db::client_t::begin_session(session_opts);
}

void gaia::db::end_session()
{
    gaia::db::client_t::end_session();
}

void gaia::db::begin_transaction()
{
    gaia::db::client_t::begin_transaction();
}

void gaia::db::rollback_transaction()
{
    gaia::db::client_t::rollback_transaction();
}

void gaia::db::commit_transaction()
{
    gaia::db::client_t::commit_transaction();
}

void gaia::db::set_commit_trigger(gaia::db::triggers::commit_trigger_fn trigger_fn)
{
    gaia::db::client_t::set_commit_trigger(trigger_fn);
}

void gaia::db::clear_shared_memory()
{
    gaia::db::client_t::clear_shared_memory();
}

gaia::db::gaia_txn_id_t gaia::db::get_txn_id()
{
    return gaia::db::client_t::get_txn_id();
}

// Implements Murmur3 64-bit finalizer
// (https://github.com/aappleby/smhasher/wiki/MurmurHash3).
// This will map 0 to 0 and acts as a pseudorandom permutation on all other integer values.
static inline uint64_t mix_bits(uint64_t x)
{
    static const uint64_t c_rotation_factor = 33ULL;
    static const uint64_t c_first_multiplication_factor = 0XFF51AFD7ED558CCDULL;
    static const uint64_t c_second_multiplication_factor = 0XC4CEB9FE1A85EC53ULL;

    x ^= x >> c_rotation_factor;
    x *= c_first_multiplication_factor;
    x ^= x >> c_rotation_factor;
    x *= c_second_multiplication_factor;
    x ^= x >> c_rotation_factor;
    return x;
}

// Publicly expose begin timestamp of current txn in obfuscated form,
// to avoid clients depending on sequential txn ids.
// Returns uint64_t since gaia_txn_id_t isn't a public type.
uint64_t gaia::db::transaction_id()
{
    auto txn_id = static_cast<uint64_t>(gaia::db::client_t::get_txn_id());
    uint64_t obfuscated_txn_id = mix_bits(txn_id);
    // We require that mix_bits() maps 0 to 0, and that its inverse does as well.
    ASSERT_INVARIANT(
        (txn_id == 0) == (obfuscated_txn_id == 0),
        "An internal txn_id of 0 must be mapped to an external txn_id of 0!");
    return obfuscated_txn_id;
}

gaia::db::session_opts_t gaia::db::get_default_session_opts()
{
    return gaia::db::session_opts_t{
        .instance_name = c_default_instance_name};
}
