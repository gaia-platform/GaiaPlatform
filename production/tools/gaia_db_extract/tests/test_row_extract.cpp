/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cstdlib>

#include <thread>

#include <gtest/gtest.h>
#include <json.hpp>

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_addr_book.h"
#include "gaia_amr_swarm.h"
#include "gaia_db_extract.hpp"

using namespace gaia::db;
using namespace gaia::direct_access;
using namespace gaia::common;
using namespace gaia::addr_book;
using namespace gaia::amr_swarm;
using namespace gaia::tools::db_extract;
using json_t = nlohmann::json;

using std::exception;
using std::string;
using std::thread;

constexpr uint32_t c_one_K_rows = 1024;
constexpr uint32_t c_just_a_few_rows = 7;

class tools__gaia_db_extract__row_extract : public db_catalog_test_base_t
{
protected:
    tools__gaia_db_extract__row_extract()
        : db_catalog_test_base_t(string("addr_book.ddl")){};

    // Utility function that creates one named employee row.
    employee_t create_employee(const char* name)
    {
        auto employee_w = employee_writer();
        employee_w.name_first = name;
        gaia_id_t id = employee_w.insert_row();
        auto employee = employee_t::get(id);
        EXPECT_STREQ(employee.name_first(), name);
        return employee;
    }

    // This function is called with a variety of parameters. It begins by inserting the
    // specified number of rows (type employee_t). Then it scans through the rows using
    // the gaia_db_extract() function to produce the JSON representation of the row
    // values. As it scans, it counts blocks and rows, verifying that the correct number
    // are found.
    void test_with_different_sizes(uint32_t number_rows_inserted, uint32_t block_size)
    {
        begin_transaction();

        // GAIAPLAT-1673 is a bug issue discovered by the 'true' section of the folloging ifdef.
        // Every time .begin() is called, a new server thread is created, plus new sockets on
        // client and server. Not only is this slower, but after 1024 rows, the following loop
        // causes the warning "Stream socket error: 'Connection reset by peer'" to be printed
        // for each deleted row. If this issue is corrected, define GAIAPLAT_1673 to run this
        // test and verify the correction.
#ifdef GAIAPLAT_1673
        // Clear out any existing rows.
        for (auto employee = *employee_t::list().begin();
             employee;
             employee = *employee_t::list().begin())
        {
            employee.delete_row();
        }
#else
        // This is the preferred method of iterating over an entire table to
        // delete its rows.
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
            string name = "Mr. " + std::to_string(i);
            create_employee(name.c_str());
        }

        commit_transaction();

        // Fetch blocks until all are gone.
        uint64_t row_id = c_start_at_first;
        int32_t row_count = 0;
        int32_t block_count = 0;
        while (true)
        {
            // Produce the JSON string of this selection.
            auto extracted_rows = gaia_db_extract("addr_book", "employee", row_id, block_size, "", c_start_at_first);
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
TEST_F(tools__gaia_db_extract__row_extract, read_blocks)
{
    // Try this with a number of permutations. The number of rows created goes from
    // 0 to over 1K. The test will scan the created rows in blocks of varying sizes
    // ranging from 0 to a few to unlimited.

    test_with_different_sizes(100, 3);
    test_with_different_sizes(0, c_just_a_few_rows);
    test_with_different_sizes(c_just_a_few_rows, c_just_a_few_rows);
    test_with_different_sizes(c_just_a_few_rows, 0);
    // It is important to run this with > c_one_K_rows.
    test_with_different_sizes(c_one_K_rows + 1, c_just_a_few_rows);
    test_with_different_sizes(c_one_K_rows + 25, c_row_limit_unlimited);
    test_with_different_sizes(c_just_a_few_rows, c_row_limit_unlimited);
}
