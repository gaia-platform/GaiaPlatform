/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "type_mapping.h"
#include "airport_demo_types.h"
#include "helpers.h"

// all definitions in this file and included files should have C linkage
extern "C" {

#include "postgres.h"
#include "flatbuffers_common_reader.h"
// flatcc generated code
#include "airport_reader.h"

// type-specific extractors
static inline Datum airport_get_gaia_id(const void *rootObject) {
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t) rootObject;
    uint64_t gaia_id = gaia_airport_airports_gaia_id(airport);
    return UInt64GetDatum(gaia_id);
}

static inline Datum airport_get_ap_id(const void *rootObject) {
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t) rootObject;
    int32_t ap_id = gaia_airport_airports_ap_id(airport);
    return Int32GetDatum(ap_id);
}

static inline Datum airport_get_name(const void *rootObject) {
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t) rootObject;
    flatbuffers_string_t name = gaia_airport_airports_name(airport);
    return flatbuffers_string_to_text_datum(name);
}

static inline Datum airport_get_city(const void *rootObject) {
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t) rootObject;
    flatbuffers_string_t city = gaia_airport_airports_city(airport);
    return flatbuffers_string_to_text_datum(city);
}

static inline Datum airport_get_country(const void *rootObject) {
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t) rootObject;
    flatbuffers_string_t country = gaia_airport_airports_name(airport);
    return flatbuffers_string_to_text_datum(country);
}

static inline Datum airport_get_iata(const void *rootObject) {
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t) rootObject;
    flatbuffers_string_t iata = gaia_airport_airports_name(airport);
    return flatbuffers_string_to_text_datum(iata);
}

static inline Datum airport_get_icao(const void *rootObject) {
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t) rootObject;
    flatbuffers_string_t icao = gaia_airport_airports_name(airport);
    return flatbuffers_string_to_text_datum(icao);
}

static inline Datum airport_get_latitude(const void *rootObject) {
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t) rootObject;
    double latitude = gaia_airport_airports_ap_id(airport);
    return Float8GetDatum(latitude);
}

static inline Datum airport_get_longitude(const void *rootObject) {
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t) rootObject;
    double longitude = gaia_airport_airports_ap_id(airport);
    return Float8GetDatum(longitude);
}

static inline Datum airport_get_altitude(const void *rootObject) {
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t) rootObject;
    int32_t altitude = gaia_airport_airports_ap_id(airport);
    return Int32GetDatum(altitude);
}

static inline Datum airport_get_timezone(const void *rootObject) {
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t) rootObject;
    float timezone = gaia_airport_airports_ap_id(airport);
    return Float4GetDatum(timezone);
}

static inline Datum airport_get_dst(const void *rootObject) {
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t) rootObject;
    flatbuffers_string_t dst = gaia_airport_airports_name(airport);
    return flatbuffers_string_to_text_datum(dst);
}

static inline Datum airport_get_tztext(const void *rootObject) {
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t) rootObject;
    flatbuffers_string_t tztext = gaia_airport_airports_name(airport);
    return flatbuffers_string_to_text_datum(tztext);
}

static inline Datum airport_get_type(const void *rootObject) {
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t) rootObject;
    flatbuffers_string_t type = gaia_airport_airports_name(airport);
    return flatbuffers_string_to_text_datum(type);
}

static inline Datum airport_get_source(const void *rootObject) {
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t) rootObject;
    flatbuffers_string_t source = gaia_airport_airports_name(airport);
    return flatbuffers_string_to_text_datum(source);
}

static inline Datum airline_get_gaia_id(const void *rootObject) {
    gaia_airport_airlines_table_t airline = (gaia_airport_airlines_table_t) rootObject;
    uint64_t gaia_id = gaia_airport_airlines_gaia_id(airline);
    return UInt64GetDatum(gaia_id);
}

static inline Datum airline_get_al_id(const void *rootObject) {
    gaia_airport_airlines_table_t airline = (gaia_airport_airlines_table_t) rootObject;
    int32_t al_id = gaia_airport_airlines_al_id(airline);
    return Int32GetDatum(al_id);
}

static inline Datum airline_get_name(const void *rootObject) {
    gaia_airport_airlines_table_t airline = (gaia_airport_airlines_table_t) rootObject;
    flatbuffers_string_t name = gaia_airport_airlines_name(airline);
    return flatbuffers_string_to_text_datum(name);
}

static inline Datum airline_get_alias(const void *rootObject) {
    gaia_airport_airlines_table_t airline = (gaia_airport_airlines_table_t) rootObject;
    flatbuffers_string_t alias = gaia_airport_airlines_alias(airline);
    return flatbuffers_string_to_text_datum(alias);
}

static inline Datum airline_get_iata(const void *rootObject) {
    gaia_airport_airlines_table_t airline = (gaia_airport_airlines_table_t) rootObject;
    flatbuffers_string_t iata = gaia_airport_airlines_iata(airline);
    return flatbuffers_string_to_text_datum(iata);
}

static inline Datum airline_get_icao(const void *rootObject) {
    gaia_airport_airlines_table_t airline = (gaia_airport_airlines_table_t) rootObject;
    flatbuffers_string_t icao = gaia_airport_airlines_icao(airline);
    return flatbuffers_string_to_text_datum(icao);
}

static inline Datum airline_get_callsign(const void *rootObject) {
    gaia_airport_airlines_table_t airline = (gaia_airport_airlines_table_t) rootObject;
    flatbuffers_string_t callsign = gaia_airport_airlines_callsign(airline);
    return flatbuffers_string_to_text_datum(callsign);
}

static inline Datum airline_get_country(const void *rootObject) {
    gaia_airport_airlines_table_t airline = (gaia_airport_airlines_table_t) rootObject;
    flatbuffers_string_t country = gaia_airport_airlines_country(airline);
    return flatbuffers_string_to_text_datum(country);
}

static inline Datum airline_get_active(const void *rootObject) {
    gaia_airport_airlines_table_t airline = (gaia_airport_airlines_table_t) rootObject;
    flatbuffers_string_t active = gaia_airport_airlines_active(airline);
    return flatbuffers_string_to_text_datum(active);
}

static inline Datum route_get_gaia_id(const void *rootObject) {
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t) rootObject;
    uint64_t gaia_id = gaia_airport_routes_gaia_id(route);
    return UInt64GetDatum(gaia_id);
}

static inline Datum route_get_gaia_al_id(const void *rootObject) {
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t) rootObject;
    uint64_t gaia_al_id = gaia_airport_routes_gaia_al_id(route);
    return UInt64GetDatum(gaia_al_id);
}

static inline Datum route_get_gaia_src_id(const void *rootObject) {
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t) rootObject;
    uint64_t gaia_src_id = gaia_airport_routes_gaia_src_id(route);
    return UInt64GetDatum(gaia_src_id);
}

static inline Datum route_get_gaia_dst_id(const void *rootObject) {
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t) rootObject;
    uint64_t gaia_dst_id = gaia_airport_routes_gaia_dst_id(route);
    return UInt64GetDatum(gaia_dst_id);
}

static inline Datum route_get_airline(const void *rootObject) {
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t) rootObject;
    flatbuffers_string_t airline = gaia_airport_routes_airline(route);
    return flatbuffers_string_to_text_datum(airline);
}

static inline Datum route_get_al_id(const void *rootObject) {
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t) rootObject;
    int32_t al_id = gaia_airport_routes_al_id(route);
    return Int32GetDatum(al_id);
}

static inline Datum route_get_src_ap(const void *rootObject) {
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t) rootObject;
    flatbuffers_string_t src_ap = gaia_airport_routes_src_ap(route);
    return flatbuffers_string_to_text_datum(src_ap);
}

static inline Datum route_get_src_ap_id(const void *rootObject) {
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t) rootObject;
    int32_t src_ap_id = gaia_airport_routes_src_ap_id(route);
    return Int32GetDatum(src_ap_id);
}

static inline Datum route_get_dst_ap(const void *rootObject) {
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t) rootObject;
    flatbuffers_string_t dst_ap = gaia_airport_routes_dst_ap(route);
    return flatbuffers_string_to_text_datum(dst_ap);
}

static inline Datum route_get_dst_ap_id(const void *rootObject) {
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t) rootObject;
    int32_t dst_ap_id = gaia_airport_routes_dst_ap_id(route);
    return Int32GetDatum(dst_ap_id);
}

static inline Datum route_get_codeshare(const void *rootObject) {
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t) rootObject;
    flatbuffers_string_t codeshare = gaia_airport_routes_codeshare(route);
    return flatbuffers_string_to_text_datum(codeshare);
}

static inline Datum route_get_stops(const void *rootObject) {
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t) rootObject;
    int32_t stops = gaia_airport_routes_stops(route);
    return Int32GetDatum(stops);
}

static inline Datum route_get_equipment(const void *rootObject) {
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t) rootObject;
    flatbuffers_string_t equipment = gaia_airport_routes_equipment(route);
    return flatbuffers_string_to_text_datum(equipment);
}

// hardcoded mappings for the demo types, later will be dynamically generated by parsing flatbuffer schema
// these arrays are defined separately so I can use sizeof() on them to determine their size at compile time
static const AttributeWithAccessor AIRPORT_ATTRS[] = {
    { "gaia_id", airport_get_gaia_id },
    { "ap_id", airport_get_ap_id },
    { "name", airport_get_name },
    { "city", airport_get_city },
    { "country", airport_get_country },
    { "iata", airport_get_iata },
    { "icao", airport_get_icao },
    { "latitude", airport_get_latitude },
    { "longitude", airport_get_longitude },
    { "altitude", airport_get_altitude },
    { "timezone", airport_get_timezone },
    { "dst", airport_get_dst },
    { "tztext", airport_get_tztext },
    { "type", airport_get_type },
    { "source", airport_get_source },
};

static const AttributeWithAccessor AIRLINE_ATTRS[] = {
    { "gaia_id", airline_get_gaia_id },
    { "al_id", airline_get_al_id },
    { "name", airline_get_name },
    { "alias", airline_get_alias },
    { "iata", airline_get_iata },
    { "icao", airline_get_icao },
    { "callsign", airline_get_callsign },
    { "country", airline_get_country },
    { "active", airline_get_active },
};

static const AttributeWithAccessor ROUTE_ATTRS[] = {
    { "gaia_id", route_get_gaia_id },
    { "gaia_al_id", route_get_gaia_al_id },
    { "gaia_src_id", route_get_gaia_src_id },
    { "gaia_dst_id", route_get_gaia_dst_id },
    { "airline", route_get_airline },
    { "al_id", route_get_al_id },
    { "src_ap", route_get_src_ap },
    { "src_ap_id", route_get_src_ap_id },
    { "dst_ap", route_get_dst_ap },
    { "dst_ap_id", route_get_dst_ap_id },
    { "codeshare", route_get_codeshare },
    { "stops", route_get_stops },
    { "equipment", route_get_equipment },
};

RelationAttributeMapping AIRPORT_MAPPING = {
    "airports",
    airport_demo_types::kAirportsType,
    false,
    (RootObjectDeserializer) gaia_airport_airports_as_root,
    AIRPORT_ATTRS,
    ARRAY_SIZE(AIRPORT_ATTRS),
};

RelationAttributeMapping AIRLINE_MAPPING = {
    "airlines",
    airport_demo_types::kAirlinesType,
    false,
    (RootObjectDeserializer) gaia_airport_airlines_as_root,
    AIRLINE_ATTRS,
    ARRAY_SIZE(AIRLINE_ATTRS),
};

RelationAttributeMapping ROUTE_MAPPING = {
    "routes",
    airport_demo_types::kRoutesType,
    true,
    (RootObjectDeserializer) gaia_airport_routes_as_root,
    ROUTE_ATTRS,
    ARRAY_SIZE(ROUTE_ATTRS),
};

const char *AIRPORT_DDL_STMT_FMT =
    "create foreign table airports( "
    "gaia_id bigint, "
    "ap_id int, name text, city text, country text, iata char(3), icao char(4), "
    "latitude double precision, longitude double precision, altitude int, "
    "timezone float, dst char(1), tztext text, type text, source text) "
    "server %s;";

const char *AIRLINE_DDL_STMT_FMT =
    "create foreign table airlines( "
    "gaia_id bigint, "
    "al_id int, "
    "name text, alias text, iata char(3), icao char(4), "
    "callsign text, country text, active char(1)) "
    "server %s;";

const char *ROUTE_DDL_STMT_FMT =
    "create foreign table routes( "
    "gaia_id bigint, gaia_al_id bigint, gaia_src_id bigint, gaia_dst_id bigint, "
    "airline text, al_id int, "
    "src_ap varchar(4), src_ap_id int, dst_ap varchar(4), dst_ap_id int, "
    "codeshare char(1), stops int, equipment text) "
    "server %s;";

} // extern "C"
