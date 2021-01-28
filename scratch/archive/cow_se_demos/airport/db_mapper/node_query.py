#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

from query import GaiaQueryFdw
from airport_query import AirportQuery as apq
from airlines_query import AirlinesQuery as alq

class NodeQuery(GaiaQueryFdw):
    def __init__(self, fdw_options, fdw_columns):
        super(NodeQuery, self).__init__(fdw_options, fdw_columns)
        self.mytype = GaiaTableMap['nodes']
        self.rowkey = 'gaia_id'
        self.node_query_types = [apd(), alq()]

    def execute(self, quals, columns, sortkeys=None):
        """ query all edge types """
        totalcnt = 0
        for nodetype in self.node_query_types:
            cnt = 0
            for result in nodetype:
                cnt += 1
                yield { 'gaia_id': result['gaia_id'],
                        'typenumber': nodetype['typenumber'],
                        'flatbuffer': None # tbd cvt flatbuffer to  nodetype.mapper
                      }
            log('Count of type %s : %s' % (nodetype['name'], totalcnt), INFO)
            totalcnt += cnt
        log('Total cnt of nodes: %s' % totalcnt, INFO)

    def insert(self, values):
        raise NotImplementedError("This FDW does not support the writable API")

    def update(self, oldvalues, newvalues):
        raise NotImplementedError("This FDW does not support the writable API")

    def delete(self, oldvalues):
        raise NotImplementedError("This FDW does not support the writable API")
