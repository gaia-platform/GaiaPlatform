/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

inline bool client_t::is_transaction_open()
{
    return (s_private_locators.is_set());
}

inline void client_t::set_commit_trigger(triggers::commit_trigger_fn trigger_fn)
{
    s_txn_commit_trigger = trigger_fn;
}

inline gaia_txn_id_t client_t::get_txn_id()
{
    return s_txn_id;
}

inline bool client_t::is_valid_event(common::gaia_type_t type)
{
    return (s_txn_commit_trigger && type < common::c_system_table_reserved_range_start);
}

inline void client_t::verify_txn_active()
{
    if (!is_transaction_open())
    {
        throw no_open_transaction();
    }
}

inline void client_t::verify_no_txn()
{
    if (is_transaction_open())
    {
        throw transaction_in_progress();
    }
}

inline void client_t::verify_session_active()
{
    if (s_session_socket == -1)
    {
        throw no_open_session();
    }
}

inline void client_t::verify_no_session()
{
    if (s_session_socket != -1)
    {
        throw session_exists();
    }
}

inline void client_t::txn_log(
    gaia_locator_t locator,
    gaia_offset_t old_offset,
    gaia_offset_t new_offset,
    gaia_operation_t operation,
    common::gaia_id_t deleted_id)
{
    ASSERT_PRECONDITION(s_log.is_set(), "Transaction log must be mapped!");

    if (operation == gaia_operation_t::remove)
    {
        ASSERT_PRECONDITION(
            deleted_id != common::c_invalid_gaia_id && new_offset == c_invalid_gaia_offset,
            "A delete operation must have a valid deleted gaia_id and an invalid new version offset!");
    }

    // We never allocate more than `c_max_log_records` records in the log.
    if (s_log.data()->record_count == c_max_log_records)
    {
        throw transaction_object_limit_exceeded();
    }

    // Initialize the new record and increment the record count.
    auto& lr = s_log.data()->log_records[s_log.data()->record_count++];
    lr.locator = locator;
    lr.old_offset = old_offset;
    lr.new_offset = new_offset;
    lr.deleted_id = deleted_id;
    lr.operation = operation;
}
