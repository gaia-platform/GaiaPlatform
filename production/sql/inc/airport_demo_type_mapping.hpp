/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "type_mapping.hpp"

// All Postgres headers and function declarations must have C linkage.
extern "C"
{

#include "postgres.h"

#include "utils/builtins.h"

} // extern "C"

// flatcc generated code.
#include "airport_q1_builder.h"
#include "airport_q1_reader.h"

// Type-specific extractors.
inline Datum airport_get_ap_id(const void* root_object)
{
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t)root_object;
    int32_t ap_id = gaia_airport_airports_ap_id(airport);
    return Int32GetDatum(ap_id);
}

inline Datum airport_get_name(const void* root_object)
{
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t)root_object;
    flatbuffers_string_t name = gaia_airport_airports_name(airport);
    return flatbuffers_string_to_text_datum(name);
}

inline Datum airport_get_city(const void* root_object)
{
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t)root_object;
    flatbuffers_string_t city = gaia_airport_airports_city(airport);
    return flatbuffers_string_to_text_datum(city);
}

inline Datum airport_get_country(const void* root_object)
{
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t)root_object;
    flatbuffers_string_t country = gaia_airport_airports_country(airport);
    return flatbuffers_string_to_text_datum(country);
}

inline Datum airport_get_iata(const void* root_object)
{
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t)root_object;
    flatbuffers_string_t iata = gaia_airport_airports_iata(airport);
    return flatbuffers_string_to_text_datum(iata);
}

inline Datum airport_get_icao(const void* root_object)
{
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t)root_object;
    flatbuffers_string_t icao = gaia_airport_airports_icao(airport);
    return flatbuffers_string_to_text_datum(icao);
}

inline Datum airport_get_latitude(const void* root_object)
{
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t)root_object;
    double latitude = gaia_airport_airports_latitude(airport);
    return Float8GetDatum(latitude);
}

inline Datum airport_get_longitude(const void* root_object)
{
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t)root_object;
    double longitude = gaia_airport_airports_longitude(airport);
    return Float8GetDatum(longitude);
}

inline Datum airport_get_altitude(const void* root_object)
{
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t)root_object;
    int32_t altitude = gaia_airport_airports_altitude(airport);
    return Int32GetDatum(altitude);
}

inline Datum airport_get_timezone(const void* root_object)
{
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t)root_object;
    float timezone = gaia_airport_airports_timezone(airport);
    return Float4GetDatum(timezone);
}

inline Datum airport_get_dst(const void* root_object)
{
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t)root_object;
    flatbuffers_string_t dst = gaia_airport_airports_dst(airport);
    return flatbuffers_string_to_text_datum(dst);
}

inline Datum airport_get_tztext(const void* root_object)
{
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t)root_object;
    flatbuffers_string_t tztext = gaia_airport_airports_tztext(airport);
    return flatbuffers_string_to_text_datum(tztext);
}

inline Datum airport_get_type(const void* root_object)
{
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t)root_object;
    flatbuffers_string_t type = gaia_airport_airports_type(airport);
    return flatbuffers_string_to_text_datum(type);
}

inline Datum airport_get_source(const void* root_object)
{
    gaia_airport_airports_table_t airport = (gaia_airport_airports_table_t)root_object;
    flatbuffers_string_t source = gaia_airport_airports_source(airport);
    return flatbuffers_string_to_text_datum(source);
}

inline Datum airline_get_al_id(const void* root_object)
{
    gaia_airport_airlines_table_t airline = (gaia_airport_airlines_table_t)root_object;
    int32_t al_id = gaia_airport_airlines_al_id(airline);
    return Int32GetDatum(al_id);
}

inline Datum airline_get_name(const void* root_object)
{
    gaia_airport_airlines_table_t airline = (gaia_airport_airlines_table_t)root_object;
    flatbuffers_string_t name = gaia_airport_airlines_name(airline);
    return flatbuffers_string_to_text_datum(name);
}

inline Datum airline_get_alias(const void* root_object)
{
    gaia_airport_airlines_table_t airline = (gaia_airport_airlines_table_t)root_object;
    flatbuffers_string_t alias = gaia_airport_airlines_alias(airline);
    return flatbuffers_string_to_text_datum(alias);
}

inline Datum airline_get_iata(const void* root_object)
{
    gaia_airport_airlines_table_t airline = (gaia_airport_airlines_table_t)root_object;
    flatbuffers_string_t iata = gaia_airport_airlines_iata(airline);
    return flatbuffers_string_to_text_datum(iata);
}

inline Datum airline_get_icao(const void* root_object)
{
    gaia_airport_airlines_table_t airline = (gaia_airport_airlines_table_t)root_object;
    flatbuffers_string_t icao = gaia_airport_airlines_icao(airline);
    return flatbuffers_string_to_text_datum(icao);
}

inline Datum airline_get_callsign(const void* root_object)
{
    gaia_airport_airlines_table_t airline = (gaia_airport_airlines_table_t)root_object;
    flatbuffers_string_t callsign = gaia_airport_airlines_callsign(airline);
    return flatbuffers_string_to_text_datum(callsign);
}

inline Datum airline_get_country(const void* root_object)
{
    gaia_airport_airlines_table_t airline = (gaia_airport_airlines_table_t)root_object;
    flatbuffers_string_t country = gaia_airport_airlines_country(airline);
    return flatbuffers_string_to_text_datum(country);
}

inline Datum airline_get_active(const void* root_object)
{
    gaia_airport_airlines_table_t airline = (gaia_airport_airlines_table_t)root_object;
    flatbuffers_string_t active = gaia_airport_airlines_active(airline);
    return flatbuffers_string_to_text_datum(active);
}

inline Datum route_get_airline(const void* root_object)
{
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t)root_object;
    flatbuffers_string_t airline = gaia_airport_routes_airline(route);
    return flatbuffers_string_to_text_datum(airline);
}

inline Datum route_get_al_id(const void* root_object)
{
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t)root_object;
    int32_t al_id = gaia_airport_routes_al_id(route);
    return Int32GetDatum(al_id);
}

inline Datum route_get_src_ap(const void* root_object)
{
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t)root_object;
    flatbuffers_string_t src_ap = gaia_airport_routes_src_ap(route);
    return flatbuffers_string_to_text_datum(src_ap);
}

inline Datum route_get_src_ap_id(const void* root_object)
{
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t)root_object;
    int32_t src_ap_id = gaia_airport_routes_src_ap_id(route);
    return Int32GetDatum(src_ap_id);
}

inline Datum route_get_dst_ap(const void* root_object)
{
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t)root_object;
    flatbuffers_string_t dst_ap = gaia_airport_routes_dst_ap(route);
    return flatbuffers_string_to_text_datum(dst_ap);
}

inline Datum route_get_dst_ap_id(const void* root_object)
{
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t)root_object;
    int32_t dst_ap_id = gaia_airport_routes_dst_ap_id(route);
    return Int32GetDatum(dst_ap_id);
}

inline Datum route_get_codeshare(const void* root_object)
{
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t)root_object;
    flatbuffers_string_t codeshare = gaia_airport_routes_codeshare(route);
    return flatbuffers_string_to_text_datum(codeshare);
}

inline Datum route_get_stops(const void* root_object)
{
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t)root_object;
    int32_t stops = gaia_airport_routes_stops(route);
    return Int32GetDatum(stops);
}

inline Datum route_get_equipment(const void* root_object)
{
    gaia_airport_routes_table_t route = (gaia_airport_routes_table_t)root_object;
    flatbuffers_string_t equipment = gaia_airport_routes_equipment(route);
    return flatbuffers_string_to_text_datum(equipment);
}

// Type-specific builders.
inline void airport_add_ap_id(flatbuffers_builder_t* builder, Datum value)
{
    int32_t ap_id = DatumGetInt32(value);
    gaia_airport_airports_ap_id_add(builder, ap_id);
}

inline void airport_add_name(flatbuffers_builder_t* builder, Datum value)
{
    const char* name = TextDatumGetCString(value);
    gaia_airport_airports_name_create_str(builder, name);
}

inline void airport_add_city(flatbuffers_builder_t* builder, Datum value)
{
    const char* city = TextDatumGetCString(value);
    gaia_airport_airports_city_create_str(builder, city);
}

inline void airport_add_country(flatbuffers_builder_t* builder, Datum value)
{
    const char* country = TextDatumGetCString(value);
    gaia_airport_airports_country_create_str(builder, country);
}

inline void airport_add_iata(flatbuffers_builder_t* builder, Datum value)
{
    const char* iata = TextDatumGetCString(value);
    gaia_airport_airports_iata_create_str(builder, iata);
}

inline void airport_add_icao(flatbuffers_builder_t* builder, Datum value)
{
    const char* icao = TextDatumGetCString(value);
    gaia_airport_airports_icao_create_str(builder, icao);
}

inline void airport_add_latitude(flatbuffers_builder_t* builder, Datum value)
{
    double latitude = DatumGetFloat8(value);
    gaia_airport_airports_latitude_add(builder, latitude);
}

inline void airport_add_longitude(flatbuffers_builder_t* builder, Datum value)
{
    double longitude = DatumGetFloat8(value);
    gaia_airport_airports_longitude_add(builder, longitude);
}

inline void airport_add_altitude(flatbuffers_builder_t* builder, Datum value)
{
    int32_t altitude = DatumGetInt32(value);
    gaia_airport_airports_altitude_add(builder, altitude);
}

inline void airport_add_timezone(flatbuffers_builder_t* builder, Datum value)
{
    float timezone = DatumGetFloat4(value);
    gaia_airport_airports_timezone_add(builder, timezone);
}

inline void airport_add_dst(flatbuffers_builder_t* builder, Datum value)
{
    const char* dst = TextDatumGetCString(value);
    gaia_airport_airports_dst_create_str(builder, dst);
}

inline void airport_add_tztext(flatbuffers_builder_t* builder, Datum value)
{
    const char* tztext = TextDatumGetCString(value);
    gaia_airport_airports_tztext_create_str(builder, tztext);
}

inline void airport_add_type(flatbuffers_builder_t* builder, Datum value)
{
    const char* type = TextDatumGetCString(value);
    gaia_airport_airports_type_create_str(builder, type);
}

inline void airport_add_source(flatbuffers_builder_t* builder, Datum value)
{
    const char* source = TextDatumGetCString(value);
    gaia_airport_airports_source_create_str(builder, source);
}

inline void airline_add_al_id(flatbuffers_builder_t* builder, Datum value)
{
    int32_t al_id = DatumGetInt32(value);
    gaia_airport_airlines_al_id_add(builder, al_id);
}

inline void airline_add_name(flatbuffers_builder_t* builder, Datum value)
{
    const char* name = TextDatumGetCString(value);
    gaia_airport_airlines_name_create_str(builder, name);
}

inline void airline_add_alias(flatbuffers_builder_t* builder, Datum value)
{
    const char* alias = TextDatumGetCString(value);
    gaia_airport_airlines_alias_create_str(builder, alias);
}

inline void airline_add_iata(flatbuffers_builder_t* builder, Datum value)
{
    const char* iata = TextDatumGetCString(value);
    gaia_airport_airlines_iata_create_str(builder, iata);
}

inline void airline_add_icao(flatbuffers_builder_t* builder, Datum value)
{
    const char* icao = TextDatumGetCString(value);
    gaia_airport_airlines_icao_create_str(builder, icao);
}

inline void airline_add_callsign(flatbuffers_builder_t* builder, Datum value)
{
    const char* callsign = TextDatumGetCString(value);
    gaia_airport_airlines_callsign_create_str(builder, callsign);
}

inline void airline_add_country(flatbuffers_builder_t* builder, Datum value)
{
    const char* country = TextDatumGetCString(value);
    gaia_airport_airlines_country_create_str(builder, country);
}

inline void airline_add_active(flatbuffers_builder_t* builder, Datum value)
{
    const char* active = TextDatumGetCString(value);
    gaia_airport_airlines_active_create_str(builder, active);
}

inline void route_add_airline(flatbuffers_builder_t* builder, Datum value)
{
    const char* airline = TextDatumGetCString(value);
    gaia_airport_routes_airline_create_str(builder, airline);
}

inline void route_add_al_id(flatbuffers_builder_t* builder, Datum value)
{
    int32_t al_id = DatumGetInt32(value);
    gaia_airport_routes_al_id_add(builder, al_id);
}

inline void route_add_src_ap(flatbuffers_builder_t* builder, Datum value)
{
    const char* src_ap = TextDatumGetCString(value);
    gaia_airport_routes_src_ap_create_str(builder, src_ap);
}

inline void route_add_src_ap_id(flatbuffers_builder_t* builder, Datum value)
{
    int32_t al_id = DatumGetInt32(value);
    gaia_airport_routes_src_ap_id_add(builder, al_id);
}

inline void route_add_dst_ap(flatbuffers_builder_t* builder, Datum value)
{
    const char* dst_ap = TextDatumGetCString(value);
    gaia_airport_routes_dst_ap_create_str(builder, dst_ap);
}

inline void route_add_dst_ap_id(flatbuffers_builder_t* builder, Datum value)
{
    int32_t dst_ap_id = DatumGetInt32(value);
    gaia_airport_routes_dst_ap_id_add(builder, dst_ap_id);
}

inline void route_add_codeshare(flatbuffers_builder_t* builder, Datum value)
{
    const char* codeshare = TextDatumGetCString(value);
    gaia_airport_routes_codeshare_create_str(builder, codeshare);
}

inline void route_add_stops(flatbuffers_builder_t* builder, Datum value)
{
    int32_t stops = DatumGetInt32(value);
    gaia_airport_routes_stops_add(builder, stops);
}

inline void route_add_equipment(flatbuffers_builder_t* builder, Datum value)
{
    const char* equipment = TextDatumGetCString(value);
    gaia_airport_routes_equipment_create_str(builder, equipment);
}

extern const relation_attribute_mapping_t c_airport_mapping;
extern const relation_attribute_mapping_t c_airline_mapping;
extern const relation_attribute_mapping_t c_route_mapping;
