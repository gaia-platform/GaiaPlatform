/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cassert>
#include <map>
#include <fstream>
#include <vector>
#include "nullable_string.hpp"
#include "airport.h" // include both flatbuffer types and object API for testing 
using namespace std;
using namespace gaia::db;
using namespace gaia::airport;

void print_airport_segments(gaia_id_t ap) {
    printf("Airport %ld:\n", ap);
    airport_src_segments airport_src_segments(ap);
    for (auto s : airport_src_segments) {
        printf("  source of segment %d - %d\n", s->id(), s->miles());
    }
    airport_dst_segments airport_dst_segments(ap);
    for (auto s : airport_dst_segments) {
        printf("  destination of segment %d - %d\n", s->id(), s->miles());
    }
}

void print_segment_trip_segments(gaia_id_t s) {
    printf("Segment %ld Trip_segments:\n", s);
    segment_trip_segments segment_trip_segments(s);
    for (auto ts : segment_trip_segments) {
        printf("  segment %s\n", ts->who());
    }
}

int main ()
{
    gaia_mem_base::init(true);

    begin_transaction();

    // first_segment
    auto f1 = Flight::insert_row(1, 0);
    auto f2 = Flight::insert_row(2, 0);
    auto f3 = Flight::insert_row(3, 0);

    // first_src_segment
    // first_dst_segment
    auto airport = new Airport();
    airport->set_name("SeaTac International");
    airport->set_city("SeaTac");
    airport->set_iata("SEA");
    airport->insert_row();
    auto ap1 = airport->gaia_id();
    airport->set_city("Seattle");
    airport->update_row();
    auto ap2 = Airport::insert_row("Denver International", "Denver", "DEN");
    auto ap3 = Airport::insert_row("Chicago O'Hare International", "Chicago", "ORD");
    auto ap4 = Airport::insert_row("LA International", "Los Angeles", "LAX");
    auto ap5 = Airport::insert_row("Atlanta International", "Atlanta", "ATL");

    auto s1 = Segment::insert_row(101, 1200, 0, 0);
    auto s2 = Segment::insert_row(102, 850,  0, 0);
    auto s3 = Segment::insert_row(103, 1000, 0, 0);
    auto s4 = Segment::insert_row(104, 1300, 0, 0);
    auto s5 = Segment::insert_row(105, 700,  0, 0);

    auto ts1 = Trip_segment::insert_row("Wayne");
    auto ts2 = Trip_segment::insert_row("Wayne");
    auto ts3 = Trip_segment::insert_row("Anita");
    auto ts4 = Trip_segment::insert_row("Anita");
    auto ts5 = Trip_segment::insert_row("Rachael");
    auto ts6 = Trip_segment::insert_row("Rachael");

    // Connect Flights to Segments.
    Segment::connect_flight(f1, s1);
    Segment::connect_flight(f1, s4);
    Segment::connect_flight(f2, s2);
    Segment::connect_flight(f2, s3);
    Segment::connect_flight(f3, s5);

    // Connect Airports to Segments.
    Segment::connect_src_airport(ap1, s1);
    Segment::connect_dst_airport(ap2, s1);
    Segment::connect_src_airport(ap4, s2);
    Segment::connect_dst_airport(ap2, s2);
    Segment::connect_src_airport(ap2, s3);
    Segment::connect_dst_airport(ap3, s3);
    Segment::connect_src_airport(ap2, s4);
    Segment::connect_dst_airport(ap5, s4);
    Segment::connect_src_airport(ap5, s5);
    Segment::connect_dst_airport(ap3, s5);

    // Connect Segments to Trip_segments.
    Trip_segment::connect_segment(s1, ts1);
    Trip_segment::connect_segment(s3, ts2);
    Trip_segment::connect_segment(s2, ts3);
    Trip_segment::connect_segment(s4, ts4);
    Trip_segment::connect_segment(s4, ts5);
    Trip_segment::connect_segment(s5, ts6);

    Airport::Iterator airports;
    Flight::Iterator flights;
    Segment::Iterator segments;
    Trip_segment::Iterator trip_segments;

    for (auto a : airports) {
        printf("airport %s - %s (%s)\n", a->name(), a->iata(), a->city());
    }
    for (auto a : flights) {
        printf("flight %d - %d\n", a->number(), a->miles_flown());
    }
    for (auto a : segments) {
        printf("segment %d - %d\n", a->id(), a->miles());
    }
    for (auto a : trip_segments) {
        printf("trip_segment %s\n", a->who());
    }

    printf("Flight %ld segments:\n", f1);
    flight_segments flight1_segments(f1);
    for (auto s : flight1_segments) {
        printf("  segment %d - %d\n", s->id(), s->miles());
        auto sap = s->src_segment();
        auto dap = s->dst_segment();
        printf("    source airport:      %s\n", sap->name());
        printf("    desgination airport: %s\n", dap->name());
    }

    printf("Flight %ld segments:\n", f2);
    auto flight2_segments = new flight_segments(f2);
    // flight_segments flight2_segments(f2);
    for (auto s : *flight2_segments) {
        printf("  segment %d - %d\n", s->id(), s->miles());
        auto sap = s->src_segment();
        auto dap = s->dst_segment();
        printf("    source airport:      %s\n", sap->name());
        printf("    desgination airport: %s\n", dap->name());
    }
    delete  flight2_segments;

    print_airport_segments(ap1);
    print_airport_segments(ap2);
    print_airport_segments(ap3);
    print_airport_segments(ap4);
    print_airport_segments(ap5);
    
    print_segment_trip_segments(s1);
    print_segment_trip_segments(s2);
    print_segment_trip_segments(s3);
    print_segment_trip_segments(s4);
    print_segment_trip_segments(s5);

    commit_transaction();

    begin_transaction();

    // The deletes allow valgrind to see any other memory issues.
    delete Flight::get_row_by_id(f1);
    delete Flight::get_row_by_id(f2);
    delete Flight::get_row_by_id(f3);

    delete Airport::get_row_by_id(ap1);
    delete Airport::get_row_by_id(ap2);
    delete Airport::get_row_by_id(ap3);
    delete Airport::get_row_by_id(ap4 );
    delete Airport::get_row_by_id(ap5 );

    delete Segment::get_row_by_id(s1);
    delete Segment::get_row_by_id(s2 );
    delete Segment::get_row_by_id(s3 );
    delete Segment::get_row_by_id(s4 );
    delete Segment::get_row_by_id(s5);

    delete Trip_segment::get_row_by_id(ts1);
    delete Trip_segment::get_row_by_id(ts2);
    delete Trip_segment::get_row_by_id(ts3);
    delete Trip_segment::get_row_by_id(ts4);
    delete Trip_segment::get_row_by_id(ts5);
    delete Trip_segment::get_row_by_id(ts6);

    commit_transaction();
}
