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

#include "io_uring_manager.hpp"
#include "wal_file.hpp"

using namespace std;
using namespace gaia::db;

void setup_dir(int* dir_fd, std::string& dirname)
{
    filesystem::remove_all(dirname);

    auto code = mkdir(dirname.c_str(), 0777);
    std::cout << "mkdir errno = " << errno << std::endl;
    assert(code == 0);

    auto fd = open(dirname.c_str(), O_DIRECTORY);
    assert(fd > 0);
    dir_fd = &fd;
}

TEST(io_uring_manager_test, single_write)
{
    std::string dirname = "persistence_log";
    int dir_fd;
    setup_dir(&dir_fd, dirname);

    size_t file_num = 1;
    int file_fd;
    auto file_size = 4 * 1024 * 1024;

    std::unique_ptr<io_uring_manager_t> io_uring_mgr = std::make_unique<io_uring_manager_t>();
    io_uring_mgr->open();
    wal_file_t wal_file(dirname, dir_fd, file_num, file_size);

    std::string file_name = wal_file.get_file_name();

    std::string to_write = "hello there";
    struct iovec iov;
    iov.iov_base = (void*)to_write.c_str();
    iov.iov_len = to_write.size();

    size_t count_submitted = io_uring_mgr->pwritev(&iov, 1, wal_file.file_fd, 0);

    count_submitted += io_uring_mgr->handle_file_close(wal_file.file_fd);
    count_submitted += io_uring_mgr->handle_submit(wal_file.file_fd, false);

    int flush_efd = io_uring_mgr->get_flush_efd();

    eventfd_t counter;

    // Block on read.
    eventfd_read(flush_efd, &counter);

    // Validate count.
    io_uring_mgr->validate_completion_batch();

    // Read file and verify.
    file_fd = open(file_name.c_str(), O_RDONLY);
    uint8_t buf[1024];
    read(file_fd, &buf, to_write.size());
    ASSERT_EQ(memcmp(&buf, (void*)to_write.c_str(), to_write.size()), 0);
    close(file_fd);

    filesystem::remove_all(dirname);
}

TEST(io_uring_manager_test, multiple_write)
{
    std::string dirname = "persistence_log";
    int dir_fd;

    setup_dir(&dir_fd, dirname);

    size_t file_num = 1;
    int file_fd;
    auto file_size = 4 * 1024 * 1024;

    std::unique_ptr<io_uring_manager_t> io_uring_mgr = std::make_unique<io_uring_manager_t>();
    size_t batch_size = 16;
    io_uring_mgr->open(batch_size);
    wal_file_t wal_file(dirname, dir_fd, file_num, file_size);

    auto file_name = wal_file.get_file_name();

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
        io_uring_mgr->pwritev(&iov_entries[i], 1, wal_file.file_fd, wal_file.current_offset);
        wal_file.allocate(to_write.size());
    }

    io_uring_mgr->handle_file_close(wal_file.file_fd);
    io_uring_mgr->handle_submit(wal_file.file_fd, false);

    int flush_efd = io_uring_mgr->get_flush_efd();
    eventfd_t counter;

    // Block on read.
    eventfd_read(flush_efd, &counter);

    // Validate count.
    io_uring_mgr->validate_completion_batch();

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

    filesystem::remove_all(dirname);
}
