/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <fcntl.h>
#include <unistd.h>

#include <cstddef>

#include <atomic>
#include <memory>
#include <string>

#include "gaia/common.hpp"

#include "db_internal_types.hpp"
#include "io_uring_manager.hpp"
#include "wal_file.hpp"

namespace gaia
{
namespace db
{

/*
 * Fill the record_header.crc field with CRC_INITIAL_VALUE when
 * computing the checksum: crc32c is vulnerable to 0-prefixing,
 * so we make sure the initial bytes are non-zero.
 */
static constexpr crc32_t c_crc_initial_value = ((uint32_t)-1);

// Class methods will be accessed via a single thread and are not thread safe.
class wal_writer_t
{
public:
    // Hardcode wal segment size for now.
    static constexpr uint64_t file_size = 4 * 1024 * 1024;

    static inline std::string dir_path{};
    static inline int dir_fd = 0;
    static inline wal_sequence_t file_num = 1;

    wal_writer_t(std::string& dir_path, int dir_fd);
    ~wal_writer_t();

    // Keep track of the current log file.
    std::unique_ptr<wal_file_t> current_file;

    std::unique_ptr<io_uring_manager_t> io_uring_manager;

    void create_txn_record(
        gaia_txn_id_t commit_ts,
        size_t payload_size,
        record_type_t type,
        std::vector<gaia_offset_t>& object_offsets,
        gaia::common::gaia_id_t* deleted_ids,
        size_t deleted_id_count);

    void process_txn_log(txn_log_t* txn_log, gaia_txn_id_t commit_ts);

    // Todo (Mihir)
    void create_decision_record(
        std::vector<std::pair<gaia_txn_id_t, txn_decision_type_t>>& txn_decisions);
};

class wal_reader_t
{
public:
    size_t get_next_wal_record(struct record_iterator* it, read_record_t* wal_record);

    size_t update_cursor(struct record_iterator* it, read_record_t* wal_record);

    void map_log_file(struct record_iterator* it, int file_fd, log_offset_t start_offset);

    void unmap_file(struct record_iterator* it);

    bool is_remaining_file_empty(uint8_t* start, uint8_t* end);

    //Todo (Mihir)
    size_t read_at_offset(int file_fd, log_offset_t offset);
    void recover_gaia_objects_from_txn_record(read_record_t* record, size_t record_size);
};

} // namespace db
} // namespace gaia
