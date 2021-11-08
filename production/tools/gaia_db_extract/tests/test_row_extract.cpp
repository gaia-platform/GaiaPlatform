/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cstdlib>

#include <iostream>
#include <thread>

#include <gtest/gtest.h>

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_addr_book.h"
#include "gaia_db_extract.hpp"
#include "json.hpp"

using namespace gaia::db;
using namespace gaia::direct_access;
using namespace gaia::common;
using namespace gaia::addr_book;
using namespace gaia::tools::db_extract;
using json_t = nlohmann::json;

using std::exception;
using std::string;
using std::thread;

constexpr uint32_t c_one_K_rows = 1024;
constexpr uint32_t c_just_a_few_rows = 7;

class row_extract_test : public db_catalog_test_base_t
{
protected:
    row_extract_test()
        : db_catalog_test_base_t(string("addr_book.ddl")){};

    // Utility function that creates one named employee row.
    employee_t create_employee(const char* name)
    {
        auto w = employee_writer();
        w.name_first = name;
        gaia_id_t id = w.insert_row();
        auto e = employee_t::get(id);
        EXPECT_STREQ(e.name_first(), name);
        return e;
    }

    void test_with_different_sizes(uint32_t number_rows_inserted, uint32_t block_size)
    {
        begin_transaction();

#ifdef GAIAPLAT_1673
        // Clear out any existing rows.
        for (auto employee = *employee_t::list().begin();
             employee;
             employee = *employee_t::list().begin())
        {
            employee.delete_row();
        }
#else
        for (auto employee_it = employee_t::list().begin();
             employee_it != employee_t::list().end();)
        {
            auto next_employee_it = employee_it++;
            (*next_employee_it).delete_row();
        }
#endif
        // Create a bunch of employee rows.
        for (uint32_t i = 0; i < number_rows_inserted; i++)
        {
            string empl_name = "Mr. " + std::to_string(i);
            create_employee(empl_name.c_str());
        }

        commit_transaction();

        // Fetch blocks until all are gone.
        uint64_t row_id = c_start_at_first;
        int32_t row_count = 0;
        int32_t block_count = 0;
        for (;;)
        {
            // Produce the JSON string of this selection.
            auto extracted_rows = gaia_db_extract("addr_book", "employee", row_id, block_size);
            if (!extracted_rows.compare(c_empty_object))
            {
                break;
            }
            ++block_count;
            // Parse the string into the json_t object to obtain the row_id.
            json_t json_object = json_t::parse(extracted_rows);
            for (auto& json_rows : json_object["rows"])
            {
                // This obtains the last seen row_id.
                row_id = json_rows["row_id"].get<uint64_t>();
                ++row_count;
            }
        }
        if (block_size > 0)
        {
            if (number_rows_inserted > 0)
            {
                ASSERT_NE(row_id, c_start_at_first);
            }
            ASSERT_EQ(row_count, number_rows_inserted);
            if (block_size != c_row_limit_unlimited)
            {
                int32_t number_blocks_expected = number_rows_inserted / block_size;
                if (number_rows_inserted % block_size)
                {
                    ++number_blocks_expected;
                }
                ASSERT_EQ(block_count, number_blocks_expected);
            }
            else
            {
                ASSERT_EQ(block_count, 1);
            }
        }
        else
        {
            ASSERT_EQ(row_count, 0);
        }
    }
};

// Store rows, convert them to JSON, scan through them as multiple blocks.
TEST_F(row_extract_test, read_blocks)
{
    // Initialization is needed when using reflection.
    ASSERT_TRUE(gaia_db_extract_initialize());

    // Try this with a number of permutations.

    test_with_different_sizes(100, 3);
    test_with_different_sizes(0, c_just_a_few_rows);
    test_with_different_sizes(c_just_a_few_rows, c_just_a_few_rows);
    test_with_different_sizes(c_just_a_few_rows, 0);
    // It is important to run this with > c_one_K_rows.
    test_with_different_sizes(c_one_K_rows + 1, c_just_a_few_rows);
    test_with_different_sizes(c_one_K_rows + 25, c_row_limit_unlimited);
    test_with_different_sizes(c_just_a_few_rows, c_row_limit_unlimited);
}
