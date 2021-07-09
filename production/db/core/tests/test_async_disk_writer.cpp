/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <filesystem>
#include <iostream>

#include "gtest/gtest.h"
#include "sys/eventfd.h"
#include <sys/stat.h>
#include <sys/types.h>

#include "gaia_internal/common/fd_helpers.hpp"

#include "async_disk_writer.hpp"
#include "persistent_log_file.hpp"

using namespace std;
using namespace gaia::db;

void setup_dir(int& dir_fd, std::string& dirname)
{
    filesystem::remove_all(dirname);

    auto code = mkdir(dirname.c_str(), 0777);
    if (code < 0)
    {
        std::cout << "error = " << errno << std::endl;
        assert(false);
    }

    auto fd = open(dirname.c_str(), O_DIRECTORY);
    assert(fd > 0);
    dir_fd = fd;
}

TEST(io_uring_manager_test, single_write)
{
    std::string dirname = "/tmp/single_write";
    int dir_fd = 0;
    setup_dir(dir_fd, dirname);
    std::cout << "DIR FD = " << dir_fd << std::endl;

    size_t file_num = 1;
    int file_fd;
    auto file_size = 4 * 1024 * 1024;

    auto validate_flush_efd = gaia::common::make_eventfd(0);
    std::unique_ptr<async_disk_writer_t> io_uring_mgr = std::make_unique<async_disk_writer_t>(validate_flush_efd, gaia::common::make_eventfd(0));
    auto set_txn_durable_fn = [=](gaia_txn_id_t) {};
    io_uring_mgr->register_txn_durable_fn(set_txn_durable_fn);
    io_uring_mgr->open();
    persistent_log_file_t wal_file(dirname, dir_fd, file_num, file_size);

    std::string file_name;
    wal_file.get_file_name(file_name);

    std::string to_write = "hello there";
    struct iovec iov;
    iov.iov_base = (void*)to_write.c_str();
    iov.iov_len = to_write.size();

    size_t count_submitted = io_uring_mgr->pwritev(&iov, 1, wal_file.get_file_fd(), 0, uring_op_t::PWRITEV_TXN);
    wal_file.allocate(to_write.size());

    count_submitted += io_uring_mgr->handle_file_close(wal_file.get_file_fd(), wal_file.get_log_file_seq());
    count_submitted += io_uring_mgr->handle_submit(wal_file.get_file_fd(), false);

    int flush_efd = io_uring_mgr->get_flush_efd();

    eventfd_t counter;

    // Block on read.
    eventfd_read(flush_efd, &counter);

    // Validate count.
    io_uring_mgr->perform_post_completion_maintenence();

    // Read file and verify.
    file_fd = open(file_name.c_str(), O_RDONLY);
    uint8_t buf[1024];
    read(file_fd, &buf, to_write.size());
    ASSERT_EQ(memcmp(&buf, (void*)to_write.c_str(), to_write.size()), 0);
    close(file_fd);
    close(validate_flush_efd);

    assert(filesystem::remove_all(dirname) > 0);
}

TEST(io_uring_manager_test, multiple_write)
{
    std::string dirname = "/tmp/multiple_write";
    int dir_fd = 0;
    setup_dir(dir_fd, dirname);
    size_t file_num = 2;
    auto file_size = 4 * 1024 * 1024;

    auto validate_flush_efd = gaia::common::make_eventfd(0);
    std::unique_ptr<async_disk_writer_t> io_uring_mgr = std::make_unique<async_disk_writer_t>(validate_flush_efd, gaia::common::make_eventfd(0));
    size_t batch_size = 16;
    auto set_txn_durable_fn = [=](gaia_txn_id_t) {};
    io_uring_mgr->register_txn_durable_fn(set_txn_durable_fn);
    io_uring_mgr->open(batch_size);
    persistent_log_file_t wal_file(dirname, dir_fd, file_num, file_size);

    std::string file_name;
    wal_file.get_file_name(file_name);

    std::string to_write = "hello there";

    // 32 batches will be flushed.
    size_t entries = 512;
    struct iovec iov_entries[entries];
    size_t total_size_written = 0;

    for (size_t i = 0; i < entries; i++)
    {
        iov_entries[i].iov_base = (void*)to_write.c_str();
        iov_entries[i].iov_len = to_write.size();
        total_size_written += to_write.size();
        io_uring_mgr->pwritev(&iov_entries[i], 1, wal_file.get_file_fd(), wal_file.get_current_offset(), uring_op_t::PWRITEV_TXN);
        wal_file.allocate(to_write.size());
    }

    io_uring_mgr->handle_file_close(wal_file.get_file_fd(), wal_file.get_log_file_seq());
    io_uring_mgr->handle_submit(wal_file.get_file_fd(), false);

    int flush_efd = io_uring_mgr->get_flush_efd();
    eventfd_t counter;

    // Block on read.
    eventfd_read(flush_efd, &counter);

    // Validate count.
    io_uring_mgr->perform_post_completion_maintenence();

    // Read file and verify.
    int verify_fd = open(file_name.c_str(), O_RDONLY);
    size_t offset_to_read = 0;
    for (size_t i = 0; i < entries; i++)
    {
        char buf[to_write.size()];
        pread(verify_fd, &buf, to_write.size(), offset_to_read);
        ASSERT_EQ(memcmp(&buf, (void*)to_write.c_str(), to_write.size()), 0);
        offset_to_read += to_write.size();
    }

    close(verify_fd);
    close(validate_flush_efd);

    assert(filesystem::remove_all(dirname) > 0);
}
