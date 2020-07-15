/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cassert>
#include <map>
#include <fstream>
#include <vector>
#include "nullable_string.hpp"
#include "gaia_airport.h" // include both flatbuffer types and object API for testing
using namespace std;
using namespace gaia::db;
using namespace gaia::airport;

void print_airport_segments(airport_t* ap) {
    printf("Airport %ld:\n", ap->gaia_id());
    for (auto s : ap->src_segment_list) {
        printf("  source of segment %d - %d\n", s->id(), s->miles());
    }
    for (auto s : ap->dst_segment_list) {
        printf("  destination of segment %d - %d\n", s->id(), s->miles());
    }
}

void print_segment_trip_segment_list(segment_t* s) {
    printf("Segment %ld Trip_segments:\n", s->gaia_id());
    for (auto ts : s->trip_segments_trip_segment_list) {
        printf("  segment %s\n", ts->who());
    }
}

int main ()
{
    gaia_mem_base::init(true);

    begin_transaction();

    // first_segment
    auto f1 = flight_t::insert_row(1, 0);
    auto f2 = flight_t::insert_row(2, 0);
    auto f3 = flight_t::insert_row(3, 0);

    // first_src_segment
    // first_dst_segment
    auto ap1 = new airport_t();
    ap1->set_name("SeaTac International");
    ap1->set_city("SeaTac");
    ap1->set_iata("SEA");
    ap1->insert_row();
    ap1->set_city("Seattle");
    ap1->update_row();
    auto ap2 = airport_t::insert_row("Denver International", "Denver", "DEN");
    auto ap3 = airport_t::insert_row("Chicago O'Hare International", "Chicago", "ORD");
    auto ap4 = airport_t::insert_row("LA International", "Los Angeles", "LAX");
    auto ap5 = airport_t::insert_row("Atlanta International", "Atlanta", "ATL");

    auto s1 = segment_t::insert_row(101, 1200, 0, 0);
    auto s2 = segment_t::insert_row(102, 850,  0, 0);
    auto s3 = segment_t::insert_row(103, 1000, 0, 0);
    auto s4 = segment_t::insert_row(104, 1300, 0, 0);
    auto s5 = segment_t::insert_row(105, 700,  0, 0);

    auto ts1 = trip_segment_t::insert_row("Wayne");
    auto ts2 = trip_segment_t::insert_row("Wayne");
    auto ts3 = trip_segment_t::insert_row("Anita");
    auto ts4 = trip_segment_t::insert_row("Anita");
    auto ts5 = trip_segment_t::insert_row("Rachael");
    auto ts6 = trip_segment_t::insert_row("Rachael");

    // Connect Flights to Segments.
    f1->flights_segment_list.insert(s1);
    f1->flights_segment_list.insert(s4);
    f2->flights_segment_list.insert(s2);
    f2->flights_segment_list.insert(s3);
    f3->flights_segment_list.insert(s5);

    // Connect Airports to Segments.
    ap1->src_segment_list.insert(s1);
    ap2->dst_segment_list.insert(s1);
    ap4->src_segment_list.insert(s2);
    ap2->dst_segment_list.insert(s2);
    ap2->src_segment_list.insert(s3);
    ap3->dst_segment_list.insert(s3);
    ap2->src_segment_list.insert(s4);
    ap5->dst_segment_list.insert(s4);
    ap5->src_segment_list.insert(s5);
    ap3->dst_segment_list.insert(s5);

    // Connect Segments to Trip_segments.
    s1->trip_segments_trip_segment_list.insert(ts1);
    s3->trip_segments_trip_segment_list.insert(ts2);
    s2->trip_segments_trip_segment_list.insert(ts3);
    s4->trip_segments_trip_segment_list.insert(ts4);
    s4->trip_segments_trip_segment_list.insert(ts5);
    s5->trip_segments_trip_segment_list.insert(ts6);

    for (auto it = airport_t::airport_table().begin(); it != airport_t::airport_table().end(); ++it)
    {
        printf("airport %s - %s (%s)\n", (*it)->name(), (*it)->iata(), (*it)->city());
    }

    for (auto a : airport_t::airport_table())
    {
        printf("airport %s - %s (%s)\n", a->name(), a->iata(), a->city());
    }

    for (auto a : flight_t::flight_table()) {
        printf("flight %d - %d\n", a->number(), a->miles_flown());
    }
    for (auto a : segment_t::segment_table()) {
        printf("segment %d - %d\n", a->id(), a->miles());
    }
    for (auto a : trip_segment_t::trip_segment_table()) {
        printf("trip_segment %s\n", a->who());
    }

    // Two forms of the same scan.
    printf("Flight %ld segments, 2 times:\n", f1->gaia_id());
    for (auto it = f1->flights_segment_list.begin(); it != f1->flights_segment_list.end(); ++it) {
        printf("  segment %d - %d\n", (*it)->id(), (*it)->miles());
        auto sap = (*it)->src_airport_owner();
        auto dap = (*it)->dst_airport_owner();
        printf("    source airport:      %s\n", sap->name());
        printf("    destination airport: %s\n", dap->name());
    }
    for (auto s : f1->flights_segment_list) {
        printf("  segment %d - %d\n", s->id(), s->miles());
        auto sap = s->src_airport_owner();
        auto dap = s->dst_airport_owner();
        printf("    source airport:      %s\n", sap->name());
        printf("    destination airport: %s\n", dap->name());
    }

    // Remove the first segment from a flight.
    printf("Removing first segment from flight 1\n");
    f1->flights_segment_list.erase(s4);
    for (auto s : f1->flights_segment_list) {
        printf("  segment %d - %d\n", s->id(), s->miles());
    }

    // Remove the first segment from a flight.
    printf("Removing other segment from flight 1\n");
    f1->flights_segment_list.erase(s1);
    for (auto s : f1->flights_segment_list) {
        printf("  segment %d - %d\n", s->id(), s->miles());
    }

    // Reconnect and remove in a different order.
    printf("Re-inserting the segments\n");
    f1->flights_segment_list.insert(s1);
    f1->flights_segment_list.insert(s4);
    for (auto s : f1->flights_segment_list) {
        printf("  segment %d - %d\n", s->id(), s->miles());
    }

    // Remove the second segment from a flight.
    printf("Removing second segment from flight 1\n");
    f1->flights_segment_list.erase(s1);
    for (auto s : f1->flights_segment_list) {
        printf("  segment %d - %d\n", s->id(), s->miles());
    }
    printf("Removing other segment from flight 1\n");
    f1->flights_segment_list.erase(s4);
    for (auto s : f1->flights_segment_list) {
        printf("  segment %d - %d\n", s->id(), s->miles());
    }

    printf("Flight %ld segments:\n", f2->gaia_id());
    for (auto s : f2->flights_segment_list) {
        printf("  segment %d - %d\n", s->id(), s->miles());
        auto sap = s->src_airport_owner();
        auto dap = s->dst_airport_owner();
        printf("    source airport:      %s\n", sap->name());
        printf("    destination airport: %s\n", dap->name());
    }

    print_airport_segments(ap1);
    print_airport_segments(ap2);
    print_airport_segments(ap3);
    print_airport_segments(ap4);
    print_airport_segments(ap5);

    print_segment_trip_segment_list(s1);
    print_segment_trip_segment_list(s2);
    print_segment_trip_segment_list(s3);
    print_segment_trip_segment_list(s4);
    print_segment_trip_segment_list(s5);

    s1->trip_segments_trip_segment_list.erase(ts1);
    ts1->delete_row();
    try {
        ap1->delete_row();
        ap2->delete_row();
        s1->delete_row();
    }
    catch (const gaia_exception& e) {
        printf("exception %s caught\n", e.what());
    }

    commit_transaction();

    begin_transaction();

    // The deletes allow valgrind to see any other memory issues.
    delete f1;
    delete f2;
    delete f3;

    delete ap1;
    delete ap2;
    delete ap3;
    delete ap4;
    delete ap5;

    delete s1;
    delete s2;
    delete s3;
    delete s4;
    delete s5;

    delete ts1;
    delete ts2;
    delete ts3;
    delete ts4;
    delete ts5;
    delete ts6;

    commit_transaction();
}
