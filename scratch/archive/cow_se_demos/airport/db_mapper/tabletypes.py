#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Static list of all types in the database
from airport_mapping import AirportMapper as APM
from airline_mapping import AirlineMapper as ALM
from route_mapping import   RouteMapper as RM

# Enums are not in pre 3.x. Multicorn uses 2.7.
def enum(**enums):
    return type('Enum', (), enums)

# nodes and edges are virtual tables that return info from other types.
Tables = enum(AIRPORTS=1, ROUTES=2, AIRLINES=3, NODES=4, EDGES=5)
GraphTypes = enum(NODE=1, EDGE = 2)

GaiaTableMap = {}
GaiaTableMap['airports'] = {
    'name': 'airports',
    'graphtype':GraphTypes.NODE,
    'mapper': APM(),
    'typenumber': Tables.AIRPORTS
    }
GaiaTableMap['airlines'] = {
    'name': 'airlines',
    'graphtype':GraphTypes.NODE,
    'mapper':ALM(),
    'typenumber': Tables.AIRLINES
}
GaiaTableMap['routes'] = {
    'name': 'routes',
    'graphtype':GraphTypes.EDGE,
    'mapper':RM(),
    'typenumber': Tables.ROUTES
}
GaiaTableMap['nodes'] = {
    'name': 'nodes',
    'graphtype':GraphTypes.NODE,
    'mapper':None,
    'typenumber': Tables.NODES
}
GaiaTableMap['edges'] = {
    'name': 'edges',
    'graphtype':GraphTypes.EDGE,
    'mapper':None,
    'typenumber': Tables.EDGES
}


