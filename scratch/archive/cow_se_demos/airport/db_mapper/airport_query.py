#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

from query import GaiaQueryFdw
from tabletypes import GaiaTableMap

class AirportQuery(GaiaQueryFdw):
    def __init__(self, fdw_options, fdw_columns):
        super(AirportQuery, self).__init__(fdw_options, fdw_columns)
        self.mytype = GaiaTableMap['airports']
        self.mapping = self.mytype['mapper'].getmapping()
        self.rowkey = 'gaia_id'
