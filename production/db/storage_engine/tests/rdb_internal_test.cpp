#include "rdb_object_converter.hpp"
#include "gtest/gtest.h"
#include "storage_engine.hpp"
#include "rdb_internal.hpp"

void check_status(rocksdb::Status s) {
    if (!s.ok()) {
        std::cerr << s.ToString();
    }
    EXPECT_TRUE(s.ok());
}

void iterate(gaia::db::rdb_internal* rdb, std::string key, std::string value) {
    rocksdb::Iterator* it = rdb->get_iterator();
    it->SeekToFirst();
    EXPECT_EQ(key, it->key().ToString());
    EXPECT_EQ(value, it->value().ToString());
    check_status(it->status());
    it->Next();
    //We've reached end of iterator when Valid() returns false.
    EXPECT_FALSE(it->Valid());
    delete it;
}

void write(gaia::db::rdb_internal* rdb, std::string key, std::string value) {
    rocksdb::WriteBatch batch1{};
    batch1.Put(key, value);
    rocksdb::Status s = rdb->write(batch1);
    check_status(s);
}

TEST(rdb_internal_test, read_write_test) {

    class rdb_internal;

    rocksdb::WriteOptions writeOptions{};
    writeOptions.disableWAL = true;

    std::string data_dir = "/tmp/db";
    gaia::db::rdb_internal* rdb = new gaia::db::rdb_internal(data_dir, writeOptions);

    rocksdb::Options options{};
    options.create_if_missing = true;

    rocksdb::Status s = rdb->open(options);
    check_status(s);

    write(rdb, "k1", "v1");

    iterate(rdb, "k1", "v1");

    // Update key
    write(rdb, "k1", "v2");

    // Iterate should dedupe keys and return a single key with value v2.
    iterate(rdb, "k1", "v2"); 

    s = rdb->close();
    check_status(s);

    // Delete data directory
    rocksdb::DestroyDB(data_dir, options);

    delete rdb;
}
