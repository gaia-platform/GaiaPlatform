#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

from query import GaiaQueryFdw
from tabletypes import GaiaTableMap

class RouteQuery(GaiaQueryFdw):
    def __init__(self, fdw_options, fdw_columns):
        super(RouteQuery, self).__init__(fdw_options, fdw_columns)
        self.mytype = GaiaTableMap['routes']
        self.mapping = self.mytype['mapper'].getmapping()
        self.rowkey = 'gaia_id' # using gaia id because the table rowkey is 3 part

    def checkspecialprops(self, d):
        """ this is an edge, need to check special props"""
        # TODO check this automatically for all edge props instead of hardcoding it here
        checkedgeprops(self, d)
