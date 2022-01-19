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
#include "log_file.hpp"
#include "memory_manager.hpp"

namespace gaia
{
namespace db
{
namespace persistence
{

/*
 * Fill the record_header.crc field with CRC_INITIAL_VALUE when
 * computing the checksum: crc32c is vulnerable to 0-prefixing,
 * so we make sure the initial bytes are non-zero.
 */
static constexpr crc32_t c_crc_initial_value = ((uint32_t)-1);

class log_handler_t
{
public:
    explicit log_handler_t(const std::string& directory_path);
    ~log_handler_t();
    void open_for_writes(int validate_flushed_batch_eventfd, int signal_checkpoint_eventfd);

    /**
     * Allocate space in the log file and return starting offset of allocation.
     */
    file_offset_t allocate_log_space(size_t payload_size);

    /**
     * Create a log record which stores txn information.
     */
    void create_txn_record(
        gaia_txn_id_t commit_ts,
        record_type_t type,
        std::vector<gaia_offset_t>& object_offsets,
        std::vector<gaia::common::gaia_id_t>& deleted_ids);

    /**
     * Process the in memory txn_log and submit the processed writes (generated log records) to the async_disk_writer.
     */
    void process_txn_log_and_write(int txn_log_fd, gaia_txn_id_t commit_ts);

    /**
     * Create a log record which stores decisions for one or more txns.
     */
    void create_decision_record(const decision_list_t& txn_decisions);

    /**
     * Submit async_disk_writer's internal I/O request queue to the kernel for processing.
     */
    void submit_writes(bool sync);

    /**
     * Validate the result of I/O calls submitted to the kernel for processing.
     */
    void validate_flushed_batch();

    /**
     * Track the session_decision_eventfd for each commit_ts; txn_commit() will only return once 
     * session_decision_eventfd is written to by the log_writer thread - signifying that the txn decision
     * has been persisted.
     */
    void register_commit_ts_for_session_notification(gaia_txn_id_t commit_ts, int session_decision_eventfd);

    /**
     * Entry point to start recovery procedure from gaia log files. Checkpointing reuses the same function.
     */
    void recover_from_persistent_log(
        gaia_txn_id_t& last_checkpointed_commit_ts,
        uint64_t& last_processed_log_seq,
        uint64_t max_log_seq_to_process,
        recovery_mode_t mode);

    /**
     * Destroy all log files with sequence number lesser than or equal to max_log_seq_to_delete.
     */
    void destroy_persistent_log(uint64_t max_log_seq_to_delete);

    /**
     * Register persistent store create/delete APIs. Rework to call persistent store APIs directly?
     */
    void register_write_to_persistent_store_fn(std::function<void(db_recovered_object_t&)> write_obj_fn);
    void register_remove_from_persistent_store_fn(std::function<void(gaia::common::gaia_id_t)> remove_obj_fn);

    /**
     * Set the log sequence counter.
     */
    void set_persistent_log_sequence(uint64_t log_seq);

    size_t get_remaining_txns_to_checkpoint_count();

private:
    // TODO: Make log file size configurable.
    static constexpr uint64_t c_file_size = 4 * 1024 * 1024;
    static constexpr std::string_view c_gaia_wal_dir_name = "/wal_dir";
    static constexpr int c_gaia_wal_dir_permissions = 0755;
    static inline std::string s_wal_dir_path{};
    static inline int s_dir_fd = -1;

    // Log file sequence starts from 1.
    static inline std::atomic<file_sequence_t::value_type> s_file_num = 1;

    // Keep track of the current log file.
    std::unique_ptr<log_file_t> m_current_file;

    std::unique_ptr<async_disk_writer_t> m_async_disk_writer;

    // Map txn commit_ts to location of log record header during recovery.
    // This index is maintained on a per log file basis. Before moving to the next file
    // we assert that this index is empty as all txns have been processed.
    // Note that the recovery implementation proceeds in increasing log file order.
    std::map<gaia_txn_id_t, uint8_t*> txn_index;

    // This map contains the current set of txns that are being processed (by either recovery or checkpointing)
    // Txns are processed one decision record at a time; a single decision record may contain
    // multiple txns.
    std::map<gaia_txn_id_t, decision_type_t> decision_index;

    gaia_txn_id_t m_max_decided_commit_ts;

    std::function<void(db_recovered_object_t&)> write_to_persistent_store_fn;
    std::function<void(gaia::common::gaia_id_t)> remove_from_persistent_store_fn;

    // Recovery & Checkpointing APIs
    size_t update_cursor(struct record_iterator_t* it);
    size_t validate_recovered_record_crc(struct record_iterator_t* it);
    void map_log_file(struct record_iterator_t* it, int file_fd, recovery_mode_t recovery_mode);
    void unmap_file(void* start, size_t size);
    bool is_remaining_file_empty(uint8_t* start, uint8_t* end);
    void write_log_record_to_persistent_store(read_record_t* record);
    void write_records(record_iterator_t* it, gaia_txn_id_t& last_checkpointed_commit_ts);
    bool write_log_file_to_persistent_store(gaia_txn_id_t& last_checkpointed_commit_ts, record_iterator_t& it);
};

} // namespace persistence
} // namespace db
} // namespace gaia
