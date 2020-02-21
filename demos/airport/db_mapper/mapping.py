#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

import flatbuffers
from AirportDemo import airlines as aa

class FBMapper(object):
    """
       Convert between three forms of a TrueGraph instance:
          (1) serialized byte form (Bytes)
          (2) Dictionary col name, value pairs (Dictionary)
          (3) Flatbuffer class instance (FB)
          This works by using the mapping from col name to triple of
              [fb col access function, fb col add function, type-str]
          fb* functions are defined by the flat buffer for getroot,
               begin serialization and end serialization.
    """
    def __init__(self):
        # define these in the type specific subclass.
        # They specify flat buffer methods, specific to the particular class
        self.mapping = {}
        self.fbGetRoot = None
        self.fbBegin = None
        self.fbEnd = None

    def BytestoFB(self, bytes, pos):
        return self.fbGetRoot(bytes,pos)

    def FBtoDictionary(self, obj):
        """ Convert all props in the flat buffer to dictionary form """
        d = {}
        for k,l in self.mapping.iteritems():
            # use the 'get value' method on the airport object
            d[k] = l[0](obj)
        return d

    def DictionarytoBytes(self, d):
        """
        Convert dictionary to flatbuffer serialized form; ignores extra props
        """
        b = flatbuffers.Builder(0)
        # Have to map the strings first, you can't mix strings and
        # regular serialization (assert on not-nested builder calls
        # for all mappings, if str type and defined in dictionary,
        # use builder to serialize in new dict. If not string, just copy.
        newdict = {}
        for k,l in self.mapping.iteritems():
            # if key in mapping not defined in dictionary, skip
            # this means that key not set
            if k not in d:
                # print "value %s not defined in dictionary" % k
                continue
            # if dictionary value is None, it's a default
            if d[k] is None:
                # print "key %s not in dictionary %s" %(k,d)
                continue
            # strings values have to be built before we start on the main object
            if l[2] == "str":
                val = b.CreateString(d[k])
            else:
                val = d[k]
            newdict[k] = val
        self.fbStart(b)
        for k,l in self.mapping.iteritems():
            if not k in newdict:
                # print "missing key " + k + " can't add it "
                continue
            # 1. use the set method on the mapping object (like routesAddSrcApId)
            # to set a value from newdict into the flatbuffer object
            # 2. floats, int64, and doubles come in as unicode, need to convert
            # them to native type.
            assert (l[2] in ['int64', 'int32', 'str', 'float', 'double']),l[2]
            if l[2] in ['float', 'double']:
                newdict[k] = float(newdict[k])
            elif l[2] in ['int64','bigint']: # should use int64 for postgres cols
                newdict[k] = int(newdict[k]) # int includes larger ints
            l[1](b, newdict[k])
        b.Finish(self.fbEnd(b))
        #return the builder, which contains the bytes and pos
        return b

    def BytestoDictionary(self, bytes, pos):
        return self.FBtoDictionary(self.BytestoFB(bytes, pos))

    def getmapping(self):
        return self.mapping
