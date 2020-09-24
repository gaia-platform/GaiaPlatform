/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "airport_demo_type_mapping.hpp"
#include "airport_demo_types.hpp"

// Hardcoded mappings for the demo types, later will be dynamically generated by
// parsing flatbuffer schema these arrays are defined separately so I can use
// sizeof() on them to determine their size at compile time.
static const attribute_t c_airport_attributes[] = {
    {"gaia_id", airport_get_gaia_id, airport_add_gaia_id},
    {"ap_id", airport_get_ap_id, airport_add_ap_id},
    {"name", airport_get_name, airport_add_name},
    {"city", airport_get_city, airport_add_city},
    {"country", airport_get_country, airport_add_country},
    {"iata", airport_get_iata, airport_add_iata},
    {"icao", airport_get_icao, airport_add_icao},
    {"latitude", airport_get_latitude, airport_add_latitude},
    {"longitude", airport_get_longitude, airport_add_longitude},
    {"altitude", airport_get_altitude, airport_add_altitude},
    {"timezone", airport_get_timezone, airport_add_timezone},
    {"dst", airport_get_dst, airport_add_dst},
    {"tztext", airport_get_tztext, airport_add_tztext},
    {"type", airport_get_type, airport_add_type},
    {"source", airport_get_source, airport_add_source},
};

static const attribute_t c_airline_attributes[] = {
    {"gaia_id", airline_get_gaia_id, airline_add_gaia_id},
    {"al_id", airline_get_al_id, airline_add_al_id},
    {"name", airline_get_name, airline_add_name},
    {"alias", airline_get_alias, airline_add_alias},
    {"iata", airline_get_iata, airline_add_iata},
    {"icao", airline_get_icao, airline_add_icao},
    {"callsign", airline_get_callsign, airline_add_callsign},
    {"country", airline_get_country, airline_add_country},
    {"active", airline_get_active, airline_add_active},
};

static const attribute_t c_route_attributes[] = {
    {"gaia_id", route_get_gaia_id, route_add_gaia_id},
    {"gaia_al_id", route_get_gaia_al_id, route_add_gaia_al_id},
    {"gaia_src_id", route_get_gaia_src_id, route_add_gaia_src_id},
    {"gaia_dst_id", route_get_gaia_dst_id, route_add_gaia_dst_id},
    {"airline", route_get_airline, route_add_airline},
    {"al_id", route_get_al_id, route_add_al_id},
    {"src_ap", route_get_src_ap, route_add_src_ap},
    {"src_ap_id", route_get_src_ap_id, route_add_src_ap_id},
    {"dst_ap", route_get_dst_ap, route_add_dst_ap},
    {"dst_ap_id", route_get_dst_ap_id, route_add_dst_ap_id},
    {"codeshare", route_get_codeshare, route_add_codeshare},
    {"stops", route_get_stops, route_add_stops},
    {"equipment", route_get_equipment, route_add_equipment},
};

const relation_attribute_mapping_t c_airport_mapping = {
    "airports",
    airport_demo_types::c_airports_type,
    gaia_airport_airports_as_root,
    gaia_airport_airports_start_as_root,
    gaia_airport_airports_end_as_root,
    c_airport_attributes,
    std::size(c_airport_attributes),
};

const relation_attribute_mapping_t c_airline_mapping = {
    "airlines",
    airport_demo_types::c_airlines_type,
    gaia_airport_airlines_as_root,
    gaia_airport_airlines_start_as_root,
    gaia_airport_airlines_end_as_root,
    c_airline_attributes,
    std::size(c_airline_attributes),
};

const relation_attribute_mapping_t c_route_mapping = {
    "routes",
    airport_demo_types::c_routes_type,
    gaia_airport_routes_as_root,
    gaia_airport_routes_start_as_root,
    gaia_airport_routes_end_as_root,
    c_route_attributes,
    std::size(c_route_attributes),
};

const char *c_airport_ddl_stmt_fmt =
    "create foreign table airports( "
    "gaia_id bigint, "
    "ap_id int, name text, city text, country text, iata char(3), icao "
    "char(4), "
    "latitude double precision, longitude double precision, altitude int, "
    "timezone float, dst char(1), tztext text, type text, source text) "
    "server %s;";

const char *c_airline_ddl_stmt_fmt =
    "create foreign table airlines( "
    "gaia_id bigint, "
    "al_id int, "
    "name text, alias text, iata char(3), icao char(5), "
    "callsign text, country text, active char(1)) "
    "server %s;";

const char *c_route_ddl_stmt_fmt =
    "create foreign table routes( "
    "gaia_id bigint, gaia_al_id bigint, gaia_src_id bigint, gaia_dst_id "
    "bigint, "
    "airline text, al_id int, "
    "src_ap varchar(4), src_ap_id int, dst_ap varchar(4), dst_ap_id int, "
    "codeshare char(1), stops int, equipment text) "
    "server %s;";
