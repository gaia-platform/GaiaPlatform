/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "persistent_store_manager.hpp"

#include <iostream>
#include <utility>

#include "rocksdb/db.h"
#include "rocksdb/write_batch.h"

#include "gaia_internal/common/system_table_types.hpp"
#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/db/gaia_db_internal.hpp"

#include "db_helpers.hpp"
#include "db_internal_types.hpp"
#include "rdb_internal.hpp"
#include "rdb_object_converter.hpp"

using namespace std;

using namespace gaia::db;
using namespace gaia::db::persistence;
using namespace gaia::common;
using namespace rocksdb;

persistent_store_manager::persistent_store_manager(
    gaia::db::counters_t* counters, gaia::db::locators_t* locators, std::string data_dir)
    : m_counters(counters), m_locators(locators)
{
    m_data_dir_path = data_dir.append(c_persistent_store_dir_name);
    rocksdb::WriteOptions write_options{};
    // Disable RocksDB write ahead log.
    write_options.disableWAL = true;
    m_rdb_internal = make_unique<gaia::db::rdb_internal_t>(m_data_dir_path.c_str(), write_options);
}

persistent_store_manager::~persistent_store_manager()
{
    close();
}

void persistent_store_manager::open()
{
    rocksdb::Options init_options{};

    // Implies 2PC log writes.
    constexpr bool c_allow_2pc = true;

    // Use fsync instead of fdatasync.
    constexpr bool c_use_fsync = true;

    // Create a new database directory if one doesn't exist.
    constexpr bool c_create_if_missing = true;

    // Size of memtable (4MB). This size isn't preallocated and is only an upper bound.
    constexpr size_t c_write_buffer_size = 4 * 1024 * 1024;

    // The maximum number of write buffers that are built up in memory.
    // For Recovery & checkpointing buffer all objects in the wal log in memory before
    // they are written to disk. This is done to avoid writing anything to the rocksdb SST in case of
    constexpr size_t c_max_write_buffer_number = INT64_MAX - 1;

    // The minimum number of memtables that will be merged together before
    // writing to storage.  If set to 1, then all memtables are flushed to L0 as
    // individual files.
    constexpr size_t c_min_write_buffer_number_to_merge = INT64_MAX - 1;

    init_options.use_fsync = c_use_fsync;
    init_options.create_if_missing = c_create_if_missing;
    init_options.write_buffer_size = c_write_buffer_size;
    init_options.max_write_buffer_number = c_max_write_buffer_number;
    init_options.min_write_buffer_number_to_merge = c_min_write_buffer_number_to_merge;

    m_rdb_internal->open_db(init_options);
}

uint64_t persistent_store_manager::get_value(const char* key)
{
    rocksdb::Slice value;
    m_rdb_internal->get(key, &value);
    gaia::db::persistence::string_reader_t value_reader(value);

    if (value.empty())
    {
        return 0;
    }

    uint64_t result;
    value_reader.read_uint64(result);
    return reinterpret_cast<uint64_t>(result);
}

void persistent_store_manager::close()
{
    m_rdb_internal->close();
}

void persistent_store_manager::flush()
{
    m_rdb_internal->flush();
}

void persistent_store_manager::update_value(const char* key, uint64_t value_to_write)
{
    string_writer_t value;
    value.write_uint64(value_to_write);
    m_rdb_internal->put(key, value.to_slice());
}

void persistent_store_manager::put(gaia::db::db_object_t& object)
{
    string_writer_t key;
    string_writer_t value;
    encode_object(&object, key, value);

    // Gaia objects encoded as key-value slices shouldn't be empty.
    ASSERT_INVARIANT(
        key.get_current_position() != 0 && value.get_current_position() != 0,
        "Failed to encode object.");
    m_rdb_internal->put(key.to_slice(), value.to_slice());
}

void persistent_store_manager::remove(gaia::common::gaia_id_t id_to_remove)
{
    // Encode key to be deleted.
    string_writer_t key;
    key.write_uint64(id_to_remove);
    ASSERT_INVARIANT(key.get_current_position() != 0, "Failed to encode object.");
    m_rdb_internal->remove(key.to_slice());
}

/**
 * This API will read the entire LSM in sorted order and construct
 * gaia_objects using the create API.
 * Additionally, this method will recover the max gaia_id and gaia_type seen by previous
 * incarnations of the database.
 *
 * Todo (Mihir) The current implementation has an issue where deleted gaia_ids may get recycled post
 * recovery. Both the last seen gaia_id & txn_id need to be
 * persisted to the RocksDB manifest. https://github.com/facebook/rocksdb/wiki/MANIFEST
 *
 * Todo(Mihir) Note that, for now we skip validating the existence of object references on recovery,
 * since these aren't validated during object creation either.
 */
void persistent_store_manager::recover(gaia_txn_id_t latest_checkpointed_commit_ts)
{
    auto it = std::unique_ptr<rocksdb::Iterator>(m_rdb_internal->get_iterator());
    gaia_id_t max_id = 0;
    gaia_type_t max_type_id = 0;

    for (it->SeekToFirst(); it->Valid(); it->Next())
    {
        if (it->key().compare(c_last_checkpointed_commit_ts_key) == 0
            || it->key().compare(c_last_processed_log_num_key) == 0)
        {
            continue;
        }

        db_object_t* recovered_object = decode_object(it->key(), it->value());
        if (recovered_object->type > max_type_id && recovered_object->type < c_system_table_reserved_range_start)
        {
            max_type_id = recovered_object->type;
        }

        if (recovered_object->id > max_id)
        {
            max_id = recovered_object->id;
        }
    }
    // Check for any errors found during the scan
    m_rdb_internal->handle_rdb_error(it->status());
    m_counters->last_id = max_id;
    m_counters->last_type_id = max_type_id;
    m_counters->last_txn_id = latest_checkpointed_commit_ts;

    // Ensure that other threads (with appropriate acquire barriers) immediately
    // observe the changed value. (This could be changed to a release barrier.)
    __sync_synchronize();
}

void persistent_store_manager::destroy_persistent_store()
{
    m_rdb_internal->destroy_persistent_store();
}
