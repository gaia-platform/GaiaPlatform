#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

from db_mapper.airport_mapping import AirportMapper
import flatbuffers
from db_mapper.AirportDemo import airports
from db_mapper.airport_query import AirportQuery
from test_aids import *

# Mapping test, Query test
#
# Mapping test:
# create an airport fb
# convert to dictionary
# convert dictionary to fb
# compare props

# constants for airport 1 (Walnut Ridge Aiport), a2 (LIT), a3 (Conway airport)


b = flatbuffers.Builder(0)

# make airport #1 Walnut Ridge
name = b.CreateString(a1rawname)
city = b.CreateString(a1rawcity) # won't be handled by current code

# create serialized form as bytes
airports.airportsStart(b)
airports.airportsAddApId(b, a1id)
airports.airportsAddName(b, name)
airports.airportsAddCity(b, city)
a = airports.airportsEnd(b)
b.Finish(a)

data = b.Bytes
pos = b.Head()

am = AirportMapper()
wr_airport = am.BytestoFB(data, pos)
asserter(a1id == wr_airport.ApId())
asserter(a1rawname == wr_airport.Name())
asserter(a1rawcity == wr_airport.City())

d = am.FBtoDictionary(wr_airport)
asserter(a1rawname == d["name"])
asserter(a1id == d["ap_id"])
# the city should not be translated
asserter(d.has_key("city"))
asserteq(len(d), 15)

# round trip it - already did dictionary to bytes, bytes to fb, fb to dictionary
# now that dictionary to bytes to flat buffer.
serialized_d_bytes = am.DictionarytoBytes(d)
wr_roundtrip = am.BytestoFB(serialized_d_bytes.Bytes, serialized_d_bytes.Head())
asserter(wr_airport.Name() == wr_roundtrip.Name())
asserter(wr_airport.ApId() == wr_roundtrip.ApId())

# Test query:
# insert 1 row
# do query
# check data
# insert  then 2 rows at once
# query for all, check they are there
# delete the second one.
# check it's gone via query
#
# d is wr_airport in dictionary form

def getelements():
    res = []
    for blah in aq.execute([],[]):
        res.append(blah)
    return res

aq = AirportQuery({}, {})
aq.begin()
asserteq(len(getelements()), 0)
result = aq.insert(d)

# check that direct access is the object, and then regular query is the object
id = result['gaia_id']
assert(isinstance(id, (int, long)))
asserteq(len(getelements()), 1)
assert(getelements()[0]['gaia_id'] == id)

cnt = 0
results = []
for r in aq.execute([],[]):
    results.append(r)
    cnt += 1
    # one result expected only
    asserteq(r['gaia_id'], id)
asserteq(cnt, 1)
r = results[0]
aq.commit()

# make sure everything is there
compareobj(d, r, am)

# make 2 more airports via fb

# TODO: skip building useless flatbuffer, wait till connect up with others.
if False:
    a2name = b.CreateString(a2rawname)
    a2city = b.CreateString(a2rawcity) # won't be handled by current code

    # create serialized form as bytes
    airports.airportsStart(b)
    airports.airportsAddApId(b, a1id)
    airports.airportsAddName(b, name)
    airports.airportsAddCity(b, city)
    a2 = airports.airportsEnd(b)
    b.Finish(a2)

a2 = make_airport(a2id, a2rawname, a2rawcity)
a3 = make_airport(a3id, a3rawname, a3rawcity)

aq.begin()
aq.insert(a2)
aq.insert(a3)
asserteq(len(getelements()), 3)

# query for all data
cnt = 0
results_by_apid = {}
allprops = am.getmapping().keys()
for r in aq.execute(allprops,[]):
    asserter('gaia_id' in r)
    # use airport id to map
    results_by_apid[r['ap_id']] = r
asserteq(len(results_by_apid), 3)
wr_dict = d
aq.commit()

compareobj(wr_dict, results_by_apid[a1id], am)
compareobj(a2, results_by_apid[a2id], am)
compareobj(a3, results_by_apid[a3id], am)

# delete by gaia_id, object 2
aq.begin()
aq.delete(results_by_apid[a2id]['gaia_id'])
asserteq(len(getelements()), 2)
results = []
for res in aq.execute(allprops, []):
    results.append(res)
aq.commit()

asserteq(len(results), 2)
asserter((results[0]['ap_id'] in  [a1id, a3id]) and
         (results[1]['ap_id'] in  [a1id, a3id]) and
         (results[1]['ap_id'] != results[0]['ap_id']))


# test handle unicode literals for bigint, float, double.
a3v2 = a3.copy()
gaia_id = aq.newID()
a3v2['gaia_id'] = unicode(gaia_id) # bigint/int64
agencymolat = 39.6608537
agencymolong = 39.6608537
timezone = -6.5

a3v2['longitude'] = unicode(agencymolong) # double
a3v2['latitude'] = unicode(agencymolat)  # double
a3v2['timezone'] = unicode(timezone) # float
aq.begin()
res = aq.insert(a3v2)
# returned value from insert is good
asserter('gaia_id' in res and isinstance((gaia_id), (int, long)))
asserter('longitude' in res and isinstance((agencymolong), float))
asserter('latitude' in res and isinstance((agencymolat), float))
asserter('timezone' in res and isinstance((timezone), float))
found = False
for res in aq.execute({},{}):
     if res['gaia_id'] == gaia_id:
         found = res
asserter(found)
# query from store and check value
asserter(isinstance(found['gaia_id'], (int, long)))
asserteq(gaia_id, found['gaia_id'])
asserter(isinstance(found['longitude'], float) and agencymolong == found['longitude'])
asserter(isinstance(found['latitude'], float) and agencymolat == found['latitude'])
asserter(isinstance(found['timezone'], float) and timezone == found['timezone'])
aq.rollback()
