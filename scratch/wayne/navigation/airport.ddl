CREATE table airport
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

CREATE table segment
(
    id int32,
    miles int32,
    status int32,
    luggage_weight int32,
    flights_ REFERENCES flight,
    src_ references airport,
    dst_ references airport
);

CREATE table trip_segment
(
    who string,
    trip_segments_ references segment
);
