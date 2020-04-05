/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "cow_se.h"

const gaia_se::gaia_type_t AIRPORTS_TYPE_ID = 1;
const gaia_se::gaia_type_t ROUTES_TYPE_ID = 2;
const gaia_se::gaia_type_t AIRLINES_TYPE_ID = 3;
const gaia_se::gaia_type_t NODES_TYPE_ID = 4;
const gaia_se::gaia_type_t EDGES_TYPE_ID = 5;

// all definitions in this file and included files should have C linkage
extern "C" {

#include "postgres.h"
// flatcc generated code
#include "airport_reader.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

// function pointer type that extracts a typed root object from a flatbuffer byte array
typedef const void *(*RootObjectDeserializer)(const void *buffer);
// function pointer type that extracts an attribute from a typed flatbuffer object
typedef Datum (*AttributeAccessor)(const void *rootObject);

// mapping of attribute names to accessor methods
typedef struct {
    const char *attr_name;
    const AttributeAccessor attr_accessor;
} AttributeWithAccessor;

// mapping of relations to attribute accessor mappings
typedef struct {
    const char *rel_name;
    gaia_se::gaia_type_t gaia_type_id;
    RootObjectDeserializer deserializer;
    const AttributeWithAccessor *attrs_with_accessors;
    size_t num_attrs;
} RelationAttributeMapping;

// flatbuffers type helpers
static Datum flatbuffers_string_to_text_datum(flatbuffers_string_t str) {
    size_t str_len = flatbuffers_string_len(str);
    size_t text_len = str_len + VARHDRSZ;
    text *t = (text *) palloc(text_len);
    SET_VARSIZE(t, text_len);
    memcpy(VARDATA(t), str, str_len);
    return CStringGetDatum(t);
}

// type-specific extractors
static inline Datum airport_get_ap_id(const void *rootObject) {
    AirportDemo_airports_table_t airport = (AirportDemo_airports_table_t) rootObject;
    int32_t ap_id = AirportDemo_airports_ap_id(airport);
    return Int32GetDatum(ap_id);
}

static inline Datum airport_get_name(const void *rootObject) {
    AirportDemo_airports_table_t airport = (AirportDemo_airports_table_t) rootObject;
    flatbuffers_string_t name = AirportDemo_airports_name(airport);
    return flatbuffers_string_to_text_datum(name);
}

static inline Datum airport_get_city(const void *rootObject) {
    AirportDemo_airports_table_t airport = (AirportDemo_airports_table_t) rootObject;
    flatbuffers_string_t city = AirportDemo_airports_city(airport);
    return flatbuffers_string_to_text_datum(city);
}

static inline Datum airport_get_country(const void *rootObject) {
    AirportDemo_airports_table_t airport = (AirportDemo_airports_table_t) rootObject;
    flatbuffers_string_t country = AirportDemo_airports_name(airport);
    return flatbuffers_string_to_text_datum(country);
}

static inline Datum airport_get_iata(const void *rootObject) {
    AirportDemo_airports_table_t airport = (AirportDemo_airports_table_t) rootObject;
    flatbuffers_string_t iata = AirportDemo_airports_name(airport);
    return flatbuffers_string_to_text_datum(iata);
}

static inline Datum airport_get_icao(const void *rootObject) {
    AirportDemo_airports_table_t airport = (AirportDemo_airports_table_t) rootObject;
    flatbuffers_string_t icao = AirportDemo_airports_name(airport);
    return flatbuffers_string_to_text_datum(icao);
}

static inline Datum airport_get_latitude(const void *rootObject) {
    AirportDemo_airports_table_t airport = (AirportDemo_airports_table_t) rootObject;
    double latitude = AirportDemo_airports_ap_id(airport);
    return Float8GetDatum(latitude);
}

static inline Datum airport_get_longitude(const void *rootObject) {
    AirportDemo_airports_table_t airport = (AirportDemo_airports_table_t) rootObject;
    double longitude = AirportDemo_airports_ap_id(airport);
    return Float8GetDatum(longitude);
}

static inline Datum airport_get_altitude(const void *rootObject) {
    AirportDemo_airports_table_t airport = (AirportDemo_airports_table_t) rootObject;
    int32_t altitude = AirportDemo_airports_ap_id(airport);
    return Int32GetDatum(altitude);
}

static inline Datum airport_get_timezone(const void *rootObject) {
    AirportDemo_airports_table_t airport = (AirportDemo_airports_table_t) rootObject;
    float timezone = AirportDemo_airports_ap_id(airport);
    return Float4GetDatum(timezone);
}

static inline Datum airport_get_dst(const void *rootObject) {
    AirportDemo_airports_table_t airport = (AirportDemo_airports_table_t) rootObject;
    flatbuffers_string_t dst = AirportDemo_airports_name(airport);
    return flatbuffers_string_to_text_datum(dst);
}

static inline Datum airport_get_tztext(const void *rootObject) {
    AirportDemo_airports_table_t airport = (AirportDemo_airports_table_t) rootObject;
    flatbuffers_string_t tztext = AirportDemo_airports_name(airport);
    return flatbuffers_string_to_text_datum(tztext);
}

static inline Datum airport_get_type(const void *rootObject) {
    AirportDemo_airports_table_t airport = (AirportDemo_airports_table_t) rootObject;
    flatbuffers_string_t type = AirportDemo_airports_name(airport);
    return flatbuffers_string_to_text_datum(type);
}

static inline Datum airport_get_source(const void *rootObject) {
    AirportDemo_airports_table_t airport = (AirportDemo_airports_table_t) rootObject;
    flatbuffers_string_t source = AirportDemo_airports_name(airport);
    return flatbuffers_string_to_text_datum(source);
}

static inline Datum airline_get_al_id(const void *rootObject) {
    AirportDemo_airlines_table_t airline = (AirportDemo_airlines_table_t) rootObject;
    int32_t al_id = AirportDemo_airlines_al_id(airline);
    return Int32GetDatum(al_id);
}

static inline Datum airline_get_name(const void *rootObject) {
    AirportDemo_airlines_table_t airline = (AirportDemo_airlines_table_t) rootObject;
    flatbuffers_string_t name = AirportDemo_airlines_name(airline);
    return flatbuffers_string_to_text_datum(name);
}

static inline Datum airline_get_alias(const void *rootObject) {
    AirportDemo_airlines_table_t airline = (AirportDemo_airlines_table_t) rootObject;
    flatbuffers_string_t alias = AirportDemo_airlines_alias(airline);
    return flatbuffers_string_to_text_datum(alias);
}

static inline Datum airline_get_iata(const void *rootObject) {
    AirportDemo_airlines_table_t airline = (AirportDemo_airlines_table_t) rootObject;
    flatbuffers_string_t iata = AirportDemo_airlines_iata(airline);
    return flatbuffers_string_to_text_datum(iata);
}

static inline Datum airline_get_icao(const void *rootObject) {
    AirportDemo_airlines_table_t airline = (AirportDemo_airlines_table_t) rootObject;
    flatbuffers_string_t icao = AirportDemo_airlines_icao(airline);
    return flatbuffers_string_to_text_datum(icao);
}

static inline Datum airline_get_callsign(const void *rootObject) {
    AirportDemo_airlines_table_t airline = (AirportDemo_airlines_table_t) rootObject;
    flatbuffers_string_t callsign = AirportDemo_airlines_callsign(airline);
    return flatbuffers_string_to_text_datum(callsign);
}

static inline Datum airline_get_country(const void *rootObject) {
    AirportDemo_airlines_table_t airline = (AirportDemo_airlines_table_t) rootObject;
    flatbuffers_string_t country = AirportDemo_airlines_country(airline);
    return flatbuffers_string_to_text_datum(country);
}

static inline Datum airline_get_active(const void *rootObject) {
    AirportDemo_airlines_table_t airline = (AirportDemo_airlines_table_t) rootObject;
    flatbuffers_string_t active = AirportDemo_airlines_active(airline);
    return flatbuffers_string_to_text_datum(active);
}

static inline Datum route_get_airline(const void *rootObject) {
    AirportDemo_routes_table_t route = (AirportDemo_routes_table_t) rootObject;
    flatbuffers_string_t airline = AirportDemo_routes_airline(route);
    return flatbuffers_string_to_text_datum(airline);
}

static inline Datum route_get_al_id(const void *rootObject) {
    AirportDemo_routes_table_t route = (AirportDemo_routes_table_t) rootObject;
    int32_t al_id = AirportDemo_routes_al_id(route);
    return Int32GetDatum(al_id);
}

static inline Datum route_get_src_ap(const void *rootObject) {
    AirportDemo_routes_table_t route = (AirportDemo_routes_table_t) rootObject;
    flatbuffers_string_t src_ap = AirportDemo_routes_src_ap(route);
    return flatbuffers_string_to_text_datum(src_ap);
}

static inline Datum route_get_src_ap_id(const void *rootObject) {
    AirportDemo_routes_table_t route = (AirportDemo_routes_table_t) rootObject;
    int32_t src_ap_id = AirportDemo_routes_src_ap_id(route);
    return Int32GetDatum(src_ap_id);
}

static inline Datum route_get_dst_ap(const void *rootObject) {
    AirportDemo_routes_table_t route = (AirportDemo_routes_table_t) rootObject;
    flatbuffers_string_t dst_ap = AirportDemo_routes_dst_ap(route);
    return flatbuffers_string_to_text_datum(dst_ap);
}

static inline Datum route_get_dst_ap_id(const void *rootObject) {
    AirportDemo_routes_table_t route = (AirportDemo_routes_table_t) rootObject;
    int32_t dst_ap_id = AirportDemo_routes_dst_ap_id(route);
    return Int32GetDatum(dst_ap_id);
}

static inline Datum route_get_codeshare(const void *rootObject) {
    AirportDemo_routes_table_t route = (AirportDemo_routes_table_t) rootObject;
    flatbuffers_string_t codeshare = AirportDemo_routes_codeshare(route);
    return flatbuffers_string_to_text_datum(codeshare);
}

static inline Datum route_get_stops(const void *rootObject) {
    AirportDemo_routes_table_t route = (AirportDemo_routes_table_t) rootObject;
    int32_t stops = AirportDemo_routes_stops(route);
    return Int32GetDatum(stops);
}

static inline Datum route_get_equipment(const void *rootObject) {
    AirportDemo_routes_table_t route = (AirportDemo_routes_table_t) rootObject;
    flatbuffers_string_t equipment = AirportDemo_routes_equipment(route);
    return flatbuffers_string_to_text_datum(equipment);
}

// hardcoded mappings for the demo types, later will be dynamically generated by parsing flatbuffer schema
// these arrays are defined separately so I can use sizeof() on them to determine their size at compile time
static const AttributeWithAccessor AIRPORT_ATTRS[] = {
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
    { "altitude", airport_get_altitude },
    { "dst", airport_get_dst },
    { "tztext", airport_get_tztext },
    { "type", airport_get_type },
    { "source", airport_get_source },
};

static const AttributeWithAccessor AIRLINE_ATTRS[] = {
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
    AIRPORTS_TYPE_ID,
    (RootObjectDeserializer) AirportDemo_airports_as_root,
    AIRPORT_ATTRS,
    ARRAY_SIZE(AIRPORT_ATTRS),
};

RelationAttributeMapping AIRLINE_MAPPING = {
    "airlines",
    AIRLINES_TYPE_ID,
    (RootObjectDeserializer) AirportDemo_airlines_as_root,
    AIRLINE_ATTRS,
    ARRAY_SIZE(AIRLINE_ATTRS),
};

RelationAttributeMapping ROUTE_MAPPING = {
    "routes",
    ROUTES_TYPE_ID,
    (RootObjectDeserializer) AirportDemo_routes_as_root,
    ROUTE_ATTRS,
    ARRAY_SIZE(ROUTE_ATTRS),
};

} // extern "C"
