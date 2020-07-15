create table airport
(
    name string,
    city string,
    iata string
);

CREATE table flight
(
    number int32,
    miles_flown int32
);

create table segment
(
    id int32,
    miles int32,
    status int32,
    luggage_weight int32,
    flights_ references flight,
    src_ references airport,
    dst_ references airport
);

create table trip_segment
(
    who string,
    trip_segments_ references segment
);
