#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

from multicorn import TransactionAwareForeignDataWrapper
from multicorn.utils import log_to_postgres, ERROR, WARNING, DEBUG, INFO
from tabletypes import GaiaTableMap, GraphTypes
from threading import Lock
from gaia.cow_se import *
import struct
import random
import base64
import json
import datetime

class GaiaQueryFdw(TransactionAwareForeignDataWrapper):
    """A foreign data wrapper that maps between flatbuffer representation and database rows.
    """

    """
    Class variable for synchronization.
    """
    lock = Lock()

    """
    Class variable for tracking initialization state.
    """
    isInitialized = False

    def __init__(self, fdw_options, fdw_columns):
        super(GaiaQueryFdw, self).__init__(fdw_options, fdw_columns)
        self.mytype = None
        self.mapping = None
        self.rowkey = None
        self.data = {} # data is a map from gaiaid to flatbuffer object
        self.islogging = True
        self.logging_level = 2 # 0 is none, 1 is call logging, 2 is results logging
        self.logfile = open("/tmp/gaiamapper.out","a")

        self.enableLocalDB = False
        self.enableGaiaDB = True

        assert(self.enableLocalDB ^ self.enableGaiaDB) # There can be only one.
        if self.enableLocalDB:
            self.log("query init local db", INFO)
        else:
            self.log("query init COW db", INFO)

        # Initialize gaia_mem_base only once per python instance.
        GaiaQueryFdw.lock.acquire()
        try:
            if self.enableGaiaDB and GaiaQueryFdw.isInitialized == False:
                print("Initializing gaia_mem_base...")
                gaia_mem_base.init(False)
                GaiaQueryFdw.isInitialized = True
            else:
                print("Skipping gaia_mem_base initialization...")
        finally:
            GaiaQueryFdw.lock.release()

    @property
    def rowid_column(self):
        """
        Returns:
            A column name which will act as a rowid column,
            for delete/update operations.

            One can think of it as a primary key.

            This can be either an existing column name, or a made-up one.
            This column name should be subsequently present in every
            returned resultset.
        """
        return self.rowkey

    def serialize_pair(self, databytes, intpos):
        """
        Serialize a pair of objects to string using a dictionary. We need the pair of value to
        rebuild the flatbuffer; the pos offset is not always 0. Current se can only store a
        single value.
        """
        assert(isinstance(intpos, (int, long)))
        d = {'b':base64.b64encode(databytes), 'i':intpos}
        return json.dumps(d)

    def deserialize_to_pair(self, str):
        """  """
        resdict = json.loads(str)
        assert('b' in resdict)
        assert('i' in resdict)
        assert(isinstance(resdict['i'], (int, long)))
        resdict['b'] = base64.b64decode(resdict['b'])
        return resdict

    def execute(self, quals, requiredcolumns, sortkeys=None):
        """Execute a query in the foreign data wrapper.

        columns: a list of the columns required on this query.
        quals: a list of predicates that can optionally be used to further restrict query
        results. They will also be used to filter results above; the format is
        triples of (column, string comparison, value), as in ("c1", ">=", "3.14").
        sortkeys are not impelemented (a list of sorted keys)
        Returns: a dictionary of results.
        """
        # if there were no columns given, return all columns (makes tests easier).
        if len(requiredcolumns) == 0:
            requiredcolumns = self.mapping.keys()
        self.log_result('+++ exec start')

        def trimcolumns(requiredcolumns, special_columns, allcolumns, rawdict):
            """ generator """
            result = {}
            # always return gaia key and rowkey
            result['gaia_id'] = rawdict['gaia_id']
            result[self.rowkey] = rawdict[self.rowkey]
            # trim to columns in required lists, ignores extra columns which shouldn't happen anyway
            for column_name in requiredcolumns:
                if column_name in special_columns and column_name not in rawdict:
                    continue
                result[column_name] = rawdict[column_name]
            return result

        self.log('query: columns: %s' % requiredcolumns, INFO)
        cnt = 0
        # there aren't required columns for queries. I can even return no columns
        #self.checkproperties(requiredcolumns, 'query')

        # TODO: remove special columns, switched to explicit gaia_src_id etc
        # aren't using these columns but they are still defined.
        special_columns = ['gaia_n', 'gaia_e']

        if self.enableLocalDB:
            for k,v in self.data.iteritems():
                cnt += 1
                # comment next 2 lines that assign v to go back to dictionary, just use v
                pairdict = self.deserialize_to_pair(v)
                v = self.mytype['mapper'].BytestoDictionary(pairdict['b'], pairdict['i'])
                line = trimcolumns(requiredcolumns, special_columns, self.mapping.keys(), v)
                self.log_result('query result %s: %s' % (cnt, line))
                yield line

        if self.enableGaiaDB:
            if (self.mytype['graphtype'] == GraphTypes.NODE):
                ptr_ptr = gaia_se_node_ptr.find_first(self.mytype['typenumber'])
            else:
                ptr_ptr = gaia_se_edge_ptr.find_first(self.mytype['typenumber'])
            while ptr_ptr.get() != None:
                cnt += 1
                if (self.mytype['graphtype'] == GraphTypes.NODE):
                    payload = get_node_payload(ptr_ptr)
                else:
                    payload = get_edge_payload(ptr_ptr)
                pairdict = self.deserialize_to_pair(payload)
                rawdict = self.mytype['mapper'].BytestoDictionary(pairdict['b'], pairdict['i'])
                line = trimcolumns(requiredcolumns, special_columns, self.mapping.keys(), rawdict)
                self.log_result('query result %s: %s' % (cnt, line))
                yield line
                ptr_ptr = ptr_ptr.find_next()
        self.log_result('+++ exec end')
        self.log('query completed: %s rows' % cnt, INFO)

    def insert(self, values):
        """
        Insert a tuple defined by ''values'' in the foreign table.

        Args:
            values (dict): a dictionary mapping column names to column values
        Returns:
            A dictionary containing the new values. These values can differ
            from the ``values`` argument if any one of them was changed
            or inserted by the foreign side. For example, if a key is auto
            generated.
        """
        self.log('insert :%s' % values, INFO)
        self.checkproperties(values, "insert")
        if 'gaia_id' not in values or values['gaia_id'] is None:
            id = self.newID()
            values['gaia_id'] = id # supposed to return fields we set like auto-keys
        else:
            id = int(values['gaia_id']) # can be unicode string, cvt if necessary
        fbbytes = self.mytype['mapper'].DictionarytoBytes(values)
        singlestr = self.serialize_pair(fbbytes.Bytes, fbbytes.Head())

        if self.enableLocalDB:
            if id in self.data:
                raise Exception("invalid insertion of existing gaia id %s" % id)
            # self.data[id] = values.copy() # uncomment to use dictionary, next line
            self.data[id] = singlestr

        if self.enableGaiaDB:
            if (self.mytype['graphtype'] == GraphTypes.NODE):
                node = create_node(id, self.mytype['typenumber'], singlestr)
            else:
                edge = create_edge(id, self.mytype['typenumber'],
                                        int(values['gaia_src_id']), int(values['gaia_dst_id']),
                                        singlestr)

        self.log_result('insert result: %s' % values)
        return values

    def update(self, oldvalues, newvalues):
        """
        Update a tuple containing ''oldvalues'' to the ''newvalues''.

        Args:
            oldvalues (dict): a dictionary mapping from column
                names to previously known values for the tuple.
            newvalues (dict): a dictionary mapping from column names to new
                values for the tuple.
        Returns:
            A dictionary containing the new values. See :method:``insert``
            for information about this return value.
        """
        self.log('update: old:%s new:%s' % (oldvalues, newvalues), INFO)

        # check basic aspects of the update (updating real props, key/id provided, not updating the key)
        self.checkproperties(oldvalues, "update")
        self.checkproperties(newvalues, "update")
        self.checkgaiaid(oldvalues)
        id = oldvalues['gaia_id']
        obj = {}
        if 'gaia_id' in newvalues and id != newvalues['gaia_id']:
            raise Exception('Not valid to change id. old values(%s). new values(%s)' % (oldvalues, newvalues))

        if self.enableLocalDB:
            if id not in self.data:
                raise Exception('Update impossible, id (%s) not in database. old values(%s). new values(%s)'
                                % (id, oldvalues, newvalues))
            # obj = self.data[id] # only if using dictionary
            # swap next few lines with self.data[i] to get back to dictionary
            pairdict = self.deserialize_to_pair(self.data[id])
            obj = self.mytype['mapper'].BytestoDictionary(pairdict['b'], pairdict['i'])

            for k in newvalues:
                obj[k] = newvalues[k]
            fbbytes = self.mytype['mapper'].DictionarytoBytes(obj)
            singlestr = self.serialize_pair(fbbytes.Bytes, fbbytes.Head())
            self.data[id] = singlestr

        if self.enableGaiaDB:
            # TODO: api that finds item by id. I have to iterate all elements in the database to find
            # the one I am updating, because the new props might not be everything.
            for obj in self.execute([], []):
                if obj['gaia_id'] == id:
                    # in the middle of a transaction in execute, so end it here.
                    # TODO: dodgy, commit outside of that other function.
                    break
            if 'gaia_id' not in obj or obj['gaia_id'] != id:
                # TODO: if I raise exception, do I need to commit trans? probably.
                raise Exception('Update impossible, id (%s) not in database. old values(%s). new values(%s)'
                                % (id, oldvalues, newvalues))
            for k in newvalues:
                obj[k] = newvalues[k]

            fbbytes = self.mytype['mapper'].DictionarytoBytes(obj)
            singlestr = self.serialize_pair(fbbytes.Bytes, fbbytes.Head())
            if (self.mytype['graphtype'] == GraphTypes.NODE):
                node = gaia_se_node.open(id)
                update_node(node, singlestr)
            else:
                edge = gaia_se_edge.open(id)
                update_edge(edge, singlestr)

        self.log_result('update: %s' % obj)
        return obj

    def delete(self, oldvalue):
        """
        Delete a tuple identified by ``oldvalue``

        Args:
            oldvalue  - the old key value. It's not a dictionary

        Returns:
            None
        """

        id = int(oldvalue)
        self.log('delete id:%s' % id, INFO)

        if self.enableLocalDB:
            if id in self.data:
                del self.data[id]
            else:
                raise Exception('object id: %s not found in store' % id)

        if self.enableGaiaDB:
            if self.mytype['graphtype'] == GraphTypes.NODE:
                node = gaia_se_node.open(id)
                gaia_se_node_ptr.remove(node)
            elif self.mytype['graphtype'] == GraphTypes.EDGE:
                edge = gaia_se_edge.open(id)
                gaia_se_edge_ptr.remove(edge)

    def pre_commit(self):
        """
        Hook called just before a commit is issued, on PostgreSQL >=9.3.
        This is where the transaction should tentatively commited.
        """
        self.log_result("+++ pre commit")

    def rollback(self):
        """
        Hook called when the transaction is rollbacked.
        """
        self.log_result("+++ rollback")
        rollback_transaction()

    def commit(self):
        """
        Hook called at commit time. On PostgreSQL >= 9.3, the pre_commit
        hook should be preferred.
        """
        self.log_result('+++ commit')
        commit_transaction()

    def end_scan(self):
        """
        Hook called at the end of a foreign scan.
        """
        self.log_result('+++ end scan')

    def end_modify(self):
        """
        Hook called at the end of a foreign modify (DML operations)
        """
        self.log_result('+++ end modify')

    def begin(self, serializable=True):
        """
        Hook called at the beginning of a transaction.
        """
        self.log_result('+++ begin tran')
        begin_transaction()

    def sub_begin(self, level):
        """
        Hook called at the beginning of a subtransaction.
        """
        self.log_result('+++ sub begin')

    def sub_rollback(self, level):
        """
        Hook called when a subtransaction is rollbacked.
        """
        self.log_result('+++ sub rollback')

    def sub_commit(self, level):
        """
        Hook called when a subtransaction is committed.
        """
        self.log_result('+++ sub-commit')

    def newID(self):
        """ generateds a random 56 bit int """
        return random.randint(1,0x7fffffffffffffff)

    def checkgaiaid(self, d):
        if 'gaia_id' not in d:
            raise Exception('This row does not contains the "gaia_id" column: (%s)' % d)
        return d['gaia_id']

    def checkedgeprops(self, d):
        """check for gaia_src_id and gaia_dst_st props, only for edge types"""
        if 'gaia_src_id' not in d:
            raise Exception('This edge does not contain the "gaia_src_id" column: (%s)' % d)
        if d['gaia_src_id'] not in self.data:
            raise Exception(
                'source node (id: %s) for this edge not in the database' % d['gaia_src_id'])
        if 'gaia_dst_id' not in d:
            raise Exception('This edge does not contain the "gaia_dst_id" column: (%s)' % d)
        if d['gaia_dst_id'] not in self.data:
            raise Exception(
                'destination node (id: %s) for this edge not in the database' % d['gaia_dst_id'])

    def checkspecialprops(self):
        """ for most types, don't need to check for any special props """
        pass

    def checkproperties(self, d, op):
        """ check all properties are valid """
        mapping = self.mytype['mapper'].getmapping()
        for p in d:
            if p not in mapping and p != 'gaia_id':
                raise Exception('When performing %s, invalid properties found: %s' %
                                    (op, p))
        # this check isn't right. you can query etc without returning gaia id fields
        if False and self.mytype['graphtype'] == GraphTypes.EDGE:
            for prop in ['gaia_src_id', 'gaia_dst_id']:
                if not prop in d:
                    raise Exception('When performing %s, node missing required field %s: object: %s' % (op, prop, d))

    def log_result(self, msg):
        self.logfile.write(str(datetime.datetime.now()))
        self.logfile.write(" ")
        self.logfile.write(msg)
        self.logfile.write("\n")
        self.logfile.flush()
        log_to_postgres(msg, DEBUG)

    def log(self, msg, logtype):
        #if self.islogging:
        self.logfile.write(str(datetime.datetime.now()))
        self.logfile.write(" ")
        self.logfile.write(msg)
        self.logfile.write("\n")
        self.logfile.flush()
        log_to_postgres(msg, DEBUG)


