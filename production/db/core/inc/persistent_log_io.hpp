/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <fcntl.h>
#include <unistd.h>

#include <cstddef>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "gaia/common.hpp"

#include "gaia_internal/db/db_object.hpp"

#include "async_disk_writer.hpp"
#include "db_internal_types.hpp"
#include "memory_manager.hpp"
#include "persistent_log_file.hpp"

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
class persistent_log_handler_t
{
private:
    // Persistent log reader.
    // Map txn commit_ts to location of log record header.
    // This index is maintained on a per file basis. Before moving to the next file
    // we assert that this index is empty as all txns would have been processed.
    std::map<gaia_txn_id_t, uint8_t*> txn_index;

    // The current set of txns that are being processed (either recovered or checkpointed)
    // Txns are processed one decision record at a time; a single decision record can contain
    // multiple txns.
    std::map<gaia_txn_id_t, txn_decision_type_t> decision_index;

    gaia_txn_id_t max_decided_commit_ts;

    size_t update_cursor(struct record_iterator_t* it);
    size_t validate_recovered_record_crc(struct record_iterator_t* it);
    void map_log_file(struct record_iterator_t* it, int file_fd, recovery_mode_t recovery_mode);
    void unmap_file(struct record_iterator_t* it);
    bool is_remaining_file_empty(uint8_t* start, uint8_t* end);
    void write_log_record_to_persistent_store(read_record_t* record);
    void index_records_in_file(record_iterator_t* it, gaia_txn_id_t last_checkpointed_commit_ts);
    bool write_log_file_to_persistent_store(std::string& wal_dir_path, uint64_t file_sequence, gaia_txn_id_t& last_checkpointed_commit_ts, recovery_mode_t recovery_mode);

    // Persistent log writer.
    // Hardcode wal segment size for now.
    static constexpr uint64_t file_size = 4 * 1024 * 1024;
    static inline std::string s_wal_dir_path{};
    static inline int dir_fd = 0;
    static inline persistent_log_sequence_t file_num = 1;

    // Keep track of the current log file.
    std::unique_ptr<persistent_log_file_t> current_file;
    std::unique_ptr<async_disk_writer_t> async_disk_writer;

    persistent_log_file_offset_t allocate_log_space(size_t payload_size);

    std::function<void(db_object_t&)> write_to_persistent_store_fn;
    std::function<void(gaia::common::gaia_id_t)> remove_from_persistent_store_fn;

public:
    persistent_log_handler_t(const std::string& directory_path);
    ~persistent_log_handler_t();
    void open_for_writes(int validate_flushed_batch_efd);

    void create_txn_record(
        gaia_txn_id_t commit_ts,
        record_type_t type,
        std::vector<gaia_offset_t>& object_offsets,
        std::vector<gaia::common::gaia_id_t>& deleted_ids);

    void process_txn_log_and_write(int txn_log_fd, gaia_txn_id_t commit_ts, gaia::db::memory_manager::memory_manager_t* memory_manager);

    void create_decision_record(decision_list_t& txn_decisions);

    void submit_writes(bool sync);

    bool batch_has_new_updates();

    void validate_flushed_batch();

    void map_commit_ts_to_session_unblock_fd(gaia_txn_id_t commit_ts, int session_unblock_fd);

    void recover_from_persistent_log(
        gaia_txn_id_t& last_checkpointed_commit_ts,
        uint64_t& last_processed_log_seq);

    void destroy_persistent_log();

    void register_write_to_persistent_store_fn(std::function<void(db_object_t&)> write_obj_fn);
    void register_remove_from_persistent_store_fn(std::function<void(gaia::common::gaia_id_t)> remove_obj_fn);

    void set_persistent_log_sequence(uint64_t log_seq);

    static constexpr std::string_view c_gaia_wal_dir_name = "/wal_dir";
};

} // namespace db
} // namespace gaia
