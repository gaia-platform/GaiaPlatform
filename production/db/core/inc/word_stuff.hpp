/*
 * Copyright 2021 Backtrace I/O, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once
#include <vector>

#include <sys/uio.h>

/**
 * The word_stuff component implements a variant of consistent
 * overhead byte stuffing (https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing,
 * https://doi.org/10.1109%2F90.769765) that replaces the forbidden
 * byte with a forbidden sequence of 2 bytes.  This is similar to
 * "word" stuffing, except that the sequence does not have to be
 * aligned; that's important to deal with the fact that short writes
 * in POSIX can fail at byte granularity.
 *
 * This functionality is expected to be used to write self-delimiting
 * records to a persistent file.  Each record should begin with a
 * 2-byte header consisting of the forbidden byte sequence (as written
 * by crdb_word_stuff_header), followed by the word-stuffed data.  One
 * can simply write that to a file in O_APPEND, and catch failures
 * after the fact: the encoding ensures that overwritten *and* lost or
 * inserted bytes only affect records that overlap with the corruption.
 *
 * On the read-side, one should scan for header byte sequences with
 * crdb_word_stuff_header_find (and assume the last record ends where
 * the file ends), and decode the stuffed contents between each
 * header.
 *
 * We use byte-sequence stuffing with a funny (0xFE 0xFD) byte
 * sequence to try and maximise the length of runs between occurrences
 * of the forbidden sequence: that makes for a workload that's more
 * efficient on contemporary machines than traditional COBS.  We chose
 * this sequence because it does not appear in any small integer,
 * unsigned or signed (two's complement), nor in any varint,
 * regardless of endianness; it also doesn't appear in any float or
 * double value with an exponent around small integers. A two-byte run
 * length header is excessive for small records, so the first run
 * length header only uses a single byte.  The worst-case space
 * overhead thus occurs for records slightly longer than 254 bytes.
  *
 * The reader always knows when it's decoding a 1-byte (when we start
 * decoding a record) or a 2-byte run size (the rest of the time), so
 * there's no ambiguity there. In both cases, the run size encodes the
 * number of literal bytes before inserting a 2-byte forbidden
 * sequence (the word_stuff header), with an escape hatch for long
 * runs without the forbidden sequence: when the run size is the
 * maximum encodable length, it encodes the number of literal bytes to
 * copy without inserting a forbidden sequence (if a forbidden
 * sequence immediately follows, we next encode a run of size 0).
 *
 * The current implementation is much more geared toward fast decoding
 * than encoding.  That's nothing inherent to the format, but simply
 * reflects the fact that we expect writes to be I/O bound for
 * durability, and reads to happen much more frequently than writes.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RADIX 0xFDUL

/**
 * We use a two-byte header.
 */
enum
{
    CRDB_WORD_STUFF_HEADER_SIZE = 2
};

/**
 * Returns a pointer to the first byte of the first occurrence of the
 * word stuffing header in `data[0 ... num - 1]`, or `data + num` if
 * none.
 */
const uint8_t* crdb_word_stuff_header_find(const uint8_t* data, size_t num);

/**
 * Returns the worst-case stuffed size for an input of `in_size` bytes.
 *
 * @param in_size the number of bytes in the raw input to stuff
 * @param with_header whether to also include the 2-byte header in the
 *   return value.
 *
 * @return the stuffed size, or SIZE_MAX on overflow.
 */
size_t crdb_word_stuffed_size(size_t in_size, bool with_header);

/**
 * CRDB_WORD_STUFFED_SIZE is a safe over-approximation of
 * crdb_word_stuffed_size that can be used as an integer constant, as
 * long as the computation does not overflow.
 *
 * We overestimate the number of run headers we might need by adding
 * *2* to `IN_SIZE / MAX_RUN_LENGTH`: we must round up, and take into
 * account the initial short run.
 */
#define CRDB_WORD_STUFFED_BOUND(IN_SIZE) \
    ((size_t)CRDB_WORD_STUFF_HEADER_SIZE + (IN_SIZE) + CRDB_WORD_STUFF_HEADER_SIZE * (2 + (IN_SIZE) / (253ULL * 253 - 1)))

/**
 * Writes the 2-byte stuffing header to `dst`, and returns a pointer
 * to `dst + CRDB_WORD_STUFF_HEADER_SIZE`.
 */
uint8_t* crdb_word_stuff_header(uint8_t dst[CRDB_WORD_STUFF_HEADER_SIZE]);

/**
 * Encode the vsrc buffer to generate a vector of iovec's that can be used one or more pwritev requests.
 * The helper buffer serves to keep buffered data on the stack till the write request returns. 
 */
void crdb_word_stuff_encode(std::vector<iovec>* txn_write_segments, uint8_t* helper_buffer, const void* vsrc, size_t src_size, bool append_header);

/**
 * Decodes the word-stuffed input in `src` into `dst`, which must have room
 * for `src_size - 1` bytes.
 *
 * @return a pointer to one past the last byte written in `dst`, or NULL on
 *   decidedly invalid input.
 */
uint8_t* crdb_word_stuff_decode(uint8_t* dst, const void* src, size_t src_size);
