/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <fcntl.h>

#include <climits>

#include <algorithm>
#include <functional>
#include <iostream>
#include <random>
#include <vector>

#include "gtest/gtest.h"
#include "sys/uio.h"
#include <sys/mman.h>
#include <sys/stat.h>

#include "stdio.h"
#include "word_stuff.hpp"

using random_bytes_engine = std::independent_bits_engine<
    std::default_random_engine, CHAR_BIT, unsigned char>;

uint8_t* generate_payload(size_t count_clashes, size_t segment_size_bytes, std::vector<uint8_t>* payload)
{
    uint8_t header[] = {RADIX + 1, RADIX};

    random_bytes_engine rbe;

    for (int i = 0; i < count_clashes; i++)
    {
        if (segment_size_bytes > 0)
        {
            std::vector<uint8_t> data(segment_size_bytes);
            std::generate(begin(data), end(data), std::ref(rbe));
            payload->insert(payload->end(), data.begin(), data.end());
        }

        // Insert clashing header.
        payload->push_back(header[0]);
        payload->push_back(header[1]);
    }

    return payload->data();
}

TEST(word_stuff, encode_and_decode)
{
    uint8_t header[] = {RADIX + 1, RADIX};
    uint8_t src[1024];
    auto src_ptr = src;
    const char* tmp_file_path = "word_stuff_test_file";

    std::vector<iovec> writes;

    // Helper buffer that will remain on the stack till the write operation is completed.
    uint8_t helper_buffer[1024];

    size_t clashes = 20;
    size_t segment_size_bytes = 20;
    std::vector<uint8_t> record;

    generate_payload(clashes, segment_size_bytes, &record);

    size_t record_size = (segment_size_bytes + 2) * clashes;
    assert(record_size == record.size());

    crdb_word_stuff_encode(&writes, &helper_buffer[0], record.data(), record_size, false);

    ASSERT_EQ(clashes * 2, writes.size());

    // Write.
    assert(std::remove(tmp_file_path) == 0);
    int file_fd = open(tmp_file_path, O_CREAT | O_RDWR, 0777);
    ASSERT_TRUE(file_fd > 0);

    auto encoded_size = pwritev(file_fd, writes.data(), writes.size(), 0);
    ASSERT_TRUE(encoded_size > 0);
    fsync(file_fd);
    close(file_fd);

    // Read
    file_fd = open(tmp_file_path, O_RDWR, 0777);
    ASSERT_TRUE(file_fd > 0);

    uint8_t to_decode[1024];
    auto ptr = (void*)&to_decode[0];
    pread(file_fd, (void*)ptr, encoded_size, 0);

    uint8_t decode_begin[1024];
    auto decode_beg_ptr = decode_begin;
    auto decode_end = crdb_word_stuff_decode(decode_begin, ptr, encoded_size);
    assert(decode_end);
    auto decoded_len = decode_end - decode_beg_ptr;
    ASSERT_EQ(decoded_len, record_size);

    // Validate decoded record is same as encoded record.
    ASSERT_EQ(0, memcmp(record.data(), decode_begin, record_size));
    close(file_fd);
}
