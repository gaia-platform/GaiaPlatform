/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>

#include "db_types.hpp"
#include "gaia_common.hpp"

namespace gaia
{
namespace db
{

// 1K oughta be enough for anybody...
constexpr size_t c_max_msg_size = 1 << 10;

enum class gaia_operation_t : uint8_t
{
    create = 0x1,
    update = 0x2,
    remove = 0x3,
    clone = 0x4
};

constexpr char c_server_connect_socket_name[] = "gaia_se_server";
constexpr char c_sch_mem_locators[] = "gaia_mem_locators";
constexpr char c_sch_mem_data[] = "gaia_mem_data";
constexpr char c_sch_mem_log[] = "gaia_mem_log";

constexpr size_t c_max_locators = 32 * 128L * 1024L;
constexpr size_t c_hash_buckets = 12289;
constexpr size_t c_hash_list_elements = c_max_locators;
constexpr size_t c_max_log_records = 1000000;
constexpr size_t c_max_objects = c_max_locators * 8;

typedef gaia_locator_t locators[c_max_locators];

struct hash_node
{
    gaia::common::gaia_id_t id;
    size_t next_offset;
    gaia::db::gaia_locator_t locator;
};

struct log
{
    size_t count;
    struct log_record
    {
        gaia::db::gaia_locator_t locator;
        gaia::db::gaia_offset_t old_offset;
        gaia::db::gaia_offset_t new_offset;
        gaia::common::gaia_id_t deleted_id;
        gaia_operation_t operation;
    } log_records[c_max_log_records];
};

struct data
{
    // The first two fields are used as cross-process atomic counters. We
    // don't need something like a cross-process mutex for this, as long as
    // we use atomic intrinsics for mutating the counters. This is because
    // the instructions targeted by the intrinsics operate at the level of
    // physical memory, not virtual addresses.
    // REVIEW: these fields should probably be changed to std::atomic<T>
    // (and the explicit calls to atomic intrinsics replaced by atomic
    // methods).
    gaia::common::gaia_id_t next_id;
    gaia::common::gaia_type_t next_type_id;
    gaia::db::gaia_txn_id_t next_txn_id;
    size_t locator_count;
    size_t hash_node_count;
    hash_node hash_nodes[c_hash_buckets + c_hash_list_elements];
    // This array is actually an untyped array of bytes, but it's defined as
    // an array of uint64_t just to enforce 8-byte alignment. Allocating
    // (c_max_locators * 8) 8-byte words for this array means we reserve 64
    // bytes on average for each object we allocate (or 1 cache line on
    // every common architecture).
    uint64_t objects[c_max_locators * 8];
};

} // namespace db
} // namespace gaia
