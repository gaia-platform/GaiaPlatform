---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

create database if not exists airport;

use airport;

create table if not exists airport
(
    name string,
    city string,
    iata string
);

create table if not exists flight
(
    number int32 unique,
    miles_flown_by_quarter int32[]
);

create table if not exists segment
(
    miles int32,
    status int8,
    luggage_weight int32
);

create table if not exists passenger (
    name string,
    address string,
    return_flight_number int32
);

create relationship if not exists segment_flight
(
    flight.segments -> segment[],
    segment.flight -> flight
);

create relationship if not exists segments_from
(
    airport.segments_from -> segment[],
    segment.src -> airport
);

create relationship if not exists segments_to
(
    airport.segments_to -> segment[],
    segment.dst -> airport
);

create table if not exists trip_segment
(
    who string
);

create relationship if not exists segment_trip
(
    segment.trip -> trip_segment,
    trip_segment.segment -> segment
);

create relationship flight_passenger
(
    flight.passengers -> passenger[],
    passenger.flight -> flight
);

create relationship return_flight_passenger
(
    flight.return_passengers -> passenger[],
    passenger.return_flight -> flight,
    using passenger(return_flight_number), flight(number)
);
