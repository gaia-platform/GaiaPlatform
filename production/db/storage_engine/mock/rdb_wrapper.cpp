/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "rocksdb/db.h"
#include "rocksdb/write_batch.h"
#include "rdb_object_converter.hpp"
#include "storage_engine.hpp"
#include "rdb_wrapper.hpp"
#include "rdb_internal.hpp"

using namespace gaia::db; 

// Temporarily hardcode this information; ideally it should come from somewhere else
static const std::string data_dir = "/tmp/db";

rdb_wrapper::rdb_wrapper() {
    rocksdb::WriteOptions writeOptions{};
    // Set write options 
    writeOptions.disableWAL = true;

    rdb_internal = new gaia::db::rdb_internal(data_dir, writeOptions);
}

rdb_wrapper::~rdb_wrapper() {
    delete rdb_internal;
}

rdb_status rdb_wrapper::open() {
    // Set options when opening rocksdb
    rocksdb::Options options;
    options.create_if_missing = true;
            
    rocksdb::Status status = rdb_internal->open(options);

    if (!status.ok()) {
        return rdb_status {status.code(), rdb_object_converter_util::get_error_message(status)};
    }
    return rdb_status {0, ""};
}

rdb_status rdb_wrapper::close() {        
    rocksdb::Status status = rdb_internal->close();

    if (!status.ok()) {
        return rdb_status {status.code(), rdb_object_converter_util::get_error_message(status)};
    }
    return rdb_status {0, ""};
}
            
void rdb_wrapper::write_on_commit(std::map<int64_t, int8_t> row_ids_with_type) {
    
    rocksdb::WriteBatch batch{};

    for (const auto element : row_ids_with_type) {
        string_writer key; 
        string_writer value;
        void* gaia_object = gaia_mem_base::offset_to_ptr(element.first);

        // Don't expect gaia object to be null
        assert(gaia_object);

        if (element.second == GaiaObjectType::node) {
            rdb_object_converter_util::encode_node(static_cast<gaia_se_node*>(gaia_object), &key, &value);
        } else {
            rdb_object_converter_util::encode_edge(static_cast<gaia_se_edge*>(gaia_object), &key, &value);
        }

        // Gaia objects encoded as key-value slices shouldn't be empty.
        assert(key.get_current_position() != 0 && value.get_current_position() != 0);

        batch.Put(key.to_slice(), value.to_slice());
    }

    rocksdb::Status status = rdb_internal->write(batch);

    if (!status.ok()) {
        std::cerr << rdb_object_converter_util::get_error_message(status);
        abort();
    }
            
}

rdb_status rdb_wrapper::read_on_recovery() {
    rocksdb::Iterator* it = rdb_internal->get_iterator();
    // Recover nodes first.
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        if (!gaia::db::rdb_object_converter_util::is_rdb_object_edge(it->value())) {
            rdb_object_converter_util::decode_node(it->key(), it->value());
        }
    }

    // Recover edges.
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        if (gaia::db::rdb_object_converter_util::is_rdb_object_edge(it->value())) {
            rdb_object_converter_util::decode_edge(it->key(), it->value());
        }
    }

    delete it; 

    return rdb_status{};
}

