# -*- coding: utf-8 -*-
"""
Base truegraphdb postgres adapter

This module contains all the python code needed by the multicorn C extension
to postgresql.

"""
import flatbuffers
from airport_mapping import AirportMapper
from airline_mapping import AirlineMapper
from route_mapping import RouteMapper
from AirportDemo import airports, airlines, routes
#import AirportDemo
from airport_query import AirportQuery
from airline_query import AirlineQuery
from route_query import RouteQuery
