/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include <gtest/gtest.h>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/ddl_execution.hpp"
#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_airport.h"
#include "gaia_parser.hpp"

using namespace gaia::airport;
using namespace gaia::catalog;
using namespace gaia::db;
using namespace std;

class gaia_generate_test : public db_catalog_test_base_t
{
protected:
    gaia_generate_test()
        : db_catalog_test_base_t("airport.ddl"){};
};

// Using the catalog manager's create_table(), create a catalog and an EDC header from that.
TEST_F(gaia_generate_test, use_create_table)
{
    create_database("airport_test");
    ddl::field_def_list_t fields;
    fields.emplace_back(make_unique<ddl::data_field_def_t>("name", data_type_t::e_string, 1));
    create_table("airport_test", "airport", fields);

    auto header_str = generate_edc_header("airport_test");
    EXPECT_NE(0, header_str.find("class airport_t"));

    auto cpp_str = generate_edc_cpp("airport_test", "gaia_airport.h");
    EXPECT_NE(0, header_str.find("trip_segment_t::insert_row"));
}

// Start from Gaia DDL to create an EDC header.
TEST_F(gaia_generate_test, parse_ddl)
{
    ddl::parser_t parser;

    EXPECT_NO_THROW(parser.parse_string("create table tmp_airport ( name string );"));
    create_database("tmp_airport");
    execute(parser.statements);

    auto header_str = generate_edc_header(c_empty_db_name);
    EXPECT_NE(0, header_str.find("class tmp_airport"));

    auto cpp_str = generate_edc_cpp(c_empty_db_name, "gaia_airport.h");
    EXPECT_NE(0, header_str.find("tmp_airport::insert_row"));
}

TEST_F(gaia_generate_test, airport_example)
{
    begin_transaction();
    // Create one segment with source and destination airports. This segment
    // flies from Denver to Chicago. A segment 888 miles long, no status, no
    // miles flown.
    const int32_t c_miles1 = 888;
    auto segment_1 = segment_t::get(segment_t::insert_row(c_miles1, 0, 0));

    // An airport.
    auto airport_1 = airport_t::get(
        airport_t::insert_row("Denver International", "Denver", "DEN"));
    auto airport_2 = airport_t::get(
        airport_t::insert_row("Chicago O'Hare International", "Chicago", "ORD"));

    // Connect the segment to the source and destination airports.
    airport_1.segments_from().insert(segment_1);
    airport_2.segments_to().insert(segment_1);
    commit_transaction();

    begin_transaction();
    // A 606 mile segment.
    const int c_miles2 = 606;
    auto segment_2 = segment_t::get(segment_t::insert_row(c_miles2, 0, 0));
    auto airport_3 = airport_t::get(
        airport_t::insert_row("Atlanta International", "Atlanta", "ATL"));
    airport_2.segments_from().insert(segment_2);
    airport_3.segments_to().insert(segment_2);

    // Create the flight #58 that spans two segments.
    const int c_flight = 58;
    auto flight_1 = flight_t::get(flight_t::insert_row(c_flight, {0}));
    // Insert both segments to the flight's list of segments.
    flight_1.segments().insert(segment_1);
    flight_1.segments().insert(segment_2);
    commit_transaction();

    begin_transaction();
    stringstream ss;
    for (const auto& flight : flight_t::list())
    {
        for (const auto& segment : flight.segments())
        {
            ss << "Segment distance: " << segment.miles() << endl;
            auto src_airport = segment.src();
            auto dst_airport = segment.dst();
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
