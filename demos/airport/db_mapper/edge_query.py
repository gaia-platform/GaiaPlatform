#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

from query import GaiaQueryFdw
from routes_query import RouteQuery as rq

class EdgeQuery(GaiaQueryFdw):
    def __init__(self, fdw_options, fdw_columns):
        super(EdgeQuery, self).__init__(fdw_options, fdw_columns)
        self.mytype = GaiaTableMap['edges']
        self.rowkey = 'gaia_id'


    # todo copy nodes query
