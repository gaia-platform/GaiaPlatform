/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

#include "catalog.hpp"
#include "db_test_base.hpp"
#include "ddl_execution.hpp"
#include "gaia_airport.h"
#include "gaia_parser.hpp"

using namespace gaia::catalog;
using namespace gaia::db;
using namespace std;

class gaia_generate_test : public db_test_base_t
{
};

// Using the catalog manager's create_table(), create a catalog and an EDC header from that.
TEST_F(gaia_generate_test, use_create_table)
{
    create_database("airport_test");
    ddl::field_def_list_t fields;
    fields.emplace_back(make_unique<ddl::data_field_def_t>("name", data_type_t::e_string, 1));
    create_table("airport_test", "airport", fields);

    auto header_str = gaia_generate("airport_test");
    EXPECT_NE(0, header_str.find("struct airport_t"));
}

// Start from Gaia DDL to create an EDC header.
TEST_F(gaia_generate_test, parse_ddl)
{
    ddl::parser_t parser;

    EXPECT_EQ(EXIT_SUCCESS, parser.parse_line("create table tmp_airport ( name string );"));
    create_database("tmp_airport");
    execute("tmp_airport", parser.statements);

    auto header_str = gaia_generate("tmp_airport");
    EXPECT_NE(0, header_str.find("struct tmp_airport_t"));
}

TEST_F(gaia_generate_test, airport_example)
{
    const char* airport_ddl_file = getenv("AIRPORT_DDL_FILE");
    ASSERT_NE(airport_ddl_file, nullptr);
    gaia::catalog::load_catalog(airport_ddl_file);

    begin_transaction();
    // Create one segment with source and destination airports. This segment
    // flies from Denver to Chicago. A segment 888 miles long, no status, no
    // miles flown.
    const int32_t c_miles1 = 888;
    auto seg = gaia::airport::segment_t::get(gaia::airport::segment_t::insert_row(c_miles1, 0, 0));
    // An airport.
    auto ap1
        = gaia::airport::airport_t::get(gaia::airport::airport_t::insert_row("Denver International", "Denver", "DEN"));
    auto ap2 = gaia::airport::airport_t::get(
        gaia::airport::airport_t::insert_row("Chicago O'Hare International", "Chicago", "ORD"));
    // Connect the segment to the source and destination airports.
    ap1.src_segment_list().insert(seg);
    ap2.dst_segment_list().insert(seg);
    commit_transaction();

    begin_transaction();
    // A 606 mile segment.
    const int c_miles2 = 606;
    auto seg2 = gaia::airport::segment_t::get(gaia::airport::segment_t::insert_row(c_miles2, 0, 0));
    auto ap3 = gaia::airport::airport_t::get(
        gaia::airport::airport_t::insert_row("Atlanta International", "Atlanta", "ATL"));
    ap2.src_segment_list().insert(seg2);
    ap3.dst_segment_list().insert(seg2);

    // Create the flight #58 that spans two segments.
    const int c_flight = 58;
    auto f1 = gaia::airport::flight_t::get(gaia::airport::flight_t::insert_row(c_flight, 0));
    // Insert both segments to the flight's list of segments.
    f1.segment_list().insert(seg);
    f1.segment_list().insert(seg2);
    commit_transaction();

    begin_transaction();
    stringstream ss;
    for (auto flight : gaia::airport::flight_t::list())
    {
        for (auto segment : flight.segment_list())
        {
            ss << "Segment distance: " << segment.miles() << endl;
            auto src_airport = segment.src_airport();
            auto dst_airport = segment.dst_airport();
            ss << "Source airport: " << src_airport.name() << endl;
            ss << "Destination airport: " << dst_airport.name() << endl
               << endl;
        }
    }
    commit_transaction();

    EXPECT_NE(ss.str().find(string("Segment distance: 888\n"
                                   "Source airport: Denver International\n"
                                   "Destination airport: Chicago O'Hare International\n")),
              string::npos);

    EXPECT_NE(ss.str().find(string("Segment distance: 606\n"
                                   "Source airport: Chicago O'Hare International\n"
                                   "Destination airport: Atlanta International\n")),
              string::npos);
}
