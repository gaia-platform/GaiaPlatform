create table airport
(
    name string,
    city string,
    iata string
);

create table flight
(
    number int32,
    miles_flown int32
);

create table segment
(
    miles int32,
    status int8,
    luggage_weight int32,
    references flight,
    src references airport,
    dst references airport
);

create table trip_segment
(
    who string,
    references segment
);
