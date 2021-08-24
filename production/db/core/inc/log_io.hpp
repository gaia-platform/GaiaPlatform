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
    log_handler_t(const std::string& directory_path);
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
    void process_txn_log_and_write(int txn_log_fd, gaia_txn_id_t commit_ts, gaia::db::memory_manager::memory_manager_t* memory_manager);

    /**
     * Create a log record which stores decisions for one or more txns.
     */
    void create_decision_record(decision_list_t& txn_decisions);

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
    void map_commit_ts_to_session_decision_eventfd(gaia_txn_id_t commit_ts, int session_decision_eventfd);

private:
    // TODO: Make log file size configurable.
    static constexpr uint64_t c_file_size = 4 * 1024 * 1024;
    static constexpr std::string_view c_gaia_wal_dir_name = "/wal_dir";
    static constexpr int c_gaia_wal_dir_permissions = 0755;
    static inline std::string s_wal_dir_path{};
    static inline int s_dir_fd = 0;

    // Log file sequence starts from 1.
    static inline file_sequence_t s_file_num = 1;

    // Keep track of the current log file.
    std::unique_ptr<log_file_t> m_current_file;

    std::unique_ptr<async_disk_writer_t> m_async_disk_writer;
};

} // namespace persistence
} // namespace db
} // namespace gaia
