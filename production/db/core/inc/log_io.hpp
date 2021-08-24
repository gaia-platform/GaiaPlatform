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

// Class methods will be accessed via a single thread and are not thread safe.
class log_handler_t
{
public:
    log_handler_t(const std::string& directory_path);
    ~log_handler_t();
    void open_for_writes(int validate_flushed_batch_efd, int signal_checkpoint_eventfd);

    file_offset_t allocate_log_space(size_t payload_size);

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

    static constexpr std::string_view c_gaia_wal_dir_name = "/wal_dir";

private:
    // Hardcode wal segment size for now.
    static constexpr uint64_t c_file_size = 4 * 1024 * 1024;
    static inline std::string s_wal_dir_path{};
    static inline int dir_fd = 0;
    static inline file_sequence_t file_num = 1;

    // Keep track of the current log file.
    std::unique_ptr<log_file_t> current_file;
    std::unique_ptr<async_disk_writer_t> async_disk_writer;

    file_offset_t file_offset_t(size_t payload_size);
};

} // namespace persistence
} // namespace db
} // namespace gaia
