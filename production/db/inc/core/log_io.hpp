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
 *
 * https://stackoverflow.com/questions/2321676/data-length-vs-crc-length
 * "From the wikipedia article: "maximal total blocklength is equal to 2r − 1". That's in bits.
 * You don't need to do much research to see that 29 - 1 is 511 bits. Using CRC-8,
 * multiple messages longer than 64 bytes will have the same CRC checksum value."
 * So CRC-16 would have max message size 2^17-1 bits or about 2^14 bytes = 16KB,
 * and CRC-32 would have max message size 2^33-1 bits or about 2^30 bytes = 1GB
 */
static constexpr crc32_t c_crc_initial_value = ((uint32_t)-1);

class log_handler_t
{
public:
    explicit log_handler_t(const std::string& data_dir_path);
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
        const std::vector<iovec>& data_iovecs,
        const std::vector<gaia::common::gaia_id_t>& deleted_ids);

    /**
     * Process the in memory txn_log and submit the processed writes (generated log records) to the async_disk_writer.
     */
    void process_txn_log_and_write(log_offset_t log_offset, gaia_txn_id_t commit_ts);

    /**
     * Create a log record which stores decisions for one or more txns.
     */
    void create_decision_record(const decision_list_t& txn_decisions);

    /**
     * Submit async_disk_writer's internal I/O request queue to the kernel for processing.
     */
    void submit_writes(bool should_wait_for_completion);

    /**
     * Validate the result of I/O calls submitted to the kernel for processing.
     */
    void perform_flushed_batch_maintenance();

private:
    static constexpr char c_gaia_wal_dir_name[] = "wal_dir";
    static constexpr int c_gaia_wal_dir_permissions = S_IRWXU | (S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH);
    static inline std::filesystem::path s_wal_dir_path{};
    static inline int s_dir_fd{-1};

    // Log file sequence starts from 1.
    static inline std::atomic<file_sequence_t::value_type> s_file_num{1};

    // Keep track of the current log file.
    std::unique_ptr<log_file_t> m_current_file;

    std::unique_ptr<async_disk_writer_t> m_async_disk_writer;
};

} // namespace persistence
} // namespace db
} // namespace gaia
