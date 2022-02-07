---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

database airport

table airport
(
    name string,
    city string,
    iata string
)

table flight
(
    number int32 unique optional,
    miles_flown_by_quarter int32[],
    segments references segment[],
    passengers references passenger[],
    return_passengers references passenger[]
)

table segment
(
    miles int32,
    status int8,
    luggage_weight int32,
    flight references flight,
    trip references trip_segment
)

table passenger (
    name string,
    address string,
    return_flight_number int32 optional,
    flight references flight using passengers,
    return_flight references flight using return_passengers
      where passenger.return_flight_number = flight.number
)

relationship segments_from
(
    airport.segments_from -> segment[],
    segment.src -> airport
)

relationship segments_to
(
    segment.dst -> airport,
    airport.segments_to -> segment[]
)

table trip_segment
(
    who string,
    segment references segment
)
