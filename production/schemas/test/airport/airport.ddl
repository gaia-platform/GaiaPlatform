---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

drop database if exists airport;

create database airport

create table airport
(
    name string,
    city string,
    iata string
)

create table flight
(
    number int32 unique,
    miles_flown_by_quarter int32[],
    segments references segment[],
    passengers references passenger[],
    return_passengers references passenger[]
)

create table segment
(
    miles int32,
    status int8,
    luggage_weight int32,
    flight references flight,
    trip references trip_segment
)

create table passenger (
    name string,
    address string,
    return_flight_number int32,
    flight references flight using passengers,
    return_flight references flight using return_passengers
      where passenger.return_flight_number = flight.number
)

create relationship segments_from
(
    airport.segments_from -> segment[],
    segment.src -> airport
)

create relationship segments_to
(
    airport.segments_to -> segment[],
    segment.dst -> airport
)

create table trip_segment
(
    who string,
    segment references segment
);
