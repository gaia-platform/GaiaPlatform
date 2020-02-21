#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

from db_mapper.airline_mapping import AirlineMapper
import flatbuffers
from db_mapper.AirportDemo import airlines
from db_mapper.airline_query import AirlineQuery
from test_aids import asserter, asserteq, compareobj


# prints each time it is called, incrementing the call
def p(i=[0]):
    i[0]+=1
    print  i[0]

# Mapping test, Query test
#
# Mapping test:
# create an airline fb
# convert to dictionary
# convert dictionary to fb
# compare props

b = flatbuffers.Builder(0)
# gaiaid = 0xcafebeef # not handled by current code
idnum = 90125 # Yes!
rawname = "Alaska Airlines"
rawicao = "ASA"
rawcallsign = "Alaska"
name = b.CreateString(rawname)
icao = b.CreateString(rawicao)
callsign = b.CreateString(rawcallsign) # won't be handled by current code
# create serialized form as bytes
airlines.airlinesStart(b)
# airlines.airlinesAddGaiaN(b, gaiaid) # not setting nested types
airlines.airlinesAddAlId(b, idnum)
airlines.airlinesAddName(b, name)
airlines.airlinesAddCallsign(b, callsign)
airlines.airlinesAddIcao(b, icao)
a = airlines.airlinesEnd(b)
b.Finish(a)

data = b.Bytes
pos = b.Head()
# init fb with it
am = AirlineMapper()
alaska_airline = am.BytestoFB(data, pos)
# asserteq(gaiaid, alaska_airline.GaiaN()) # not using id for now
asserter(idnum == alaska_airline.AlId())
asserter(rawname == alaska_airline.Name())
asserter(rawicao == alaska_airline.Icao())
asserter(rawcallsign == alaska_airline.Callsign())

d = am.FBtoDictionary(alaska_airline)
asserter(rawname == d["name"])
asserter(idnum == d["al_id"])
asserter(rawicao == d["icao"])
asserter(d.has_key("callsign"))
asserteq(len(d), 9)

serialized_d_bytes = am.DictionarytoBytes(d)
alaska_roundtrip = am.BytestoFB(serialized_d_bytes.Bytes, serialized_d_bytes.Head())
# asserter(alaska_airline.GaiaN() == alaska_roundtrip.GaiaN())
asserter(alaska_airline.Name() == alaska_roundtrip.Name())
asserter(alaska_airline.AlId() == alaska_roundtrip.AlId())
asserter(alaska_airline.Alias() == alaska_roundtrip.Alias())
asserter(alaska_airline.Iata() == alaska_roundtrip.Iata())
asserter(alaska_airline.Icao() == alaska_roundtrip.Icao())
asserter(alaska_airline.Callsign() == alaska_roundtrip.Callsign())
asserter(alaska_airline.Country() == alaska_roundtrip.Country())
asserter(alaska_airline.Active() == alaska_roundtrip.Active())

# Test query:
# insert 1 row
# do query
# check data
# insert  then 2 rows at once
# query for all, check they are there
# delete the second one.
# check it's gone via query
# To be done: Insert an airport record, so there are mixed records
# Verify only an airport record comes back.
def getelements():
    res = []
    for blah in aq.execute([],[]):
        res.append(blah)
    return res
# d is alaska_airline in dictionary form
aq = AirlineQuery({},{})
aq.begin()
asserteq(len(getelements()), 0) # its empty
result = aq.insert(d)
# check that direct access is the object, and then regular query is the object
id = result['gaia_id']
assert(isinstance(id, (int, long)))
asserteq(len(getelements()), 1)
asserteq(id, getelements()[0]['gaia_id'])
aq.commit()

cnt = 0
results = []
aq.begin()
for r in aq.execute([],[]):
    results.append(r)
    cnt += 1
    # one result expected only
    asserteq(r['gaia_id'], id)
aq.commit()
asserteq(cnt, 1)
r = results[0]
# make sure everything is there
compareobj(d, r, am)

# make 2 more airlines
a2id = 99
a2rawname = "Northwest Airlines"
a2rawicao = "NWA"
a2rawcallsign = "Northwest"
a2name = b.CreateString(a2rawname)
a2icao = b.CreateString(a2rawicao)
a2callsign = b.CreateString(a2rawcallsign) # won't be handled by current code

airlines.airlinesStart(b)
airlines.airlinesAddAlId(b, a2id)
airlines.airlinesAddName(b, a2name)
airlines.airlinesAddCallsign(b, a2callsign)
airlines.airlinesAddIcao(b, a2icao)
a2fb = airlines.airlinesEnd(b)
b.Finish(a2fb)
a2bytes = b.Bytes
a2pos = b.Head()
a2fb = am.BytestoFB(a2bytes, a2pos)
a2d = am.FBtoDictionary(a2fb)
a2d['gaia_id'] = None

# construct a3d directly just to do it differently.
# make sure all props at least have none set
a3d = {}
for p in am.getmapping().keys():
    a3d[p] = None
a3id = 1492
a3rawname = "Neutral Airport"
a3rawcallsign = "000"
a3d['al_id'] = a3id
a3d['name'] = a3rawname
a3d['callsign'] = a3rawcallsign
aq.begin()
aq.insert(a2d)
aq.insert(a3d)
asserteq(len(getelements()), 3)

# query for all data
cnt = 0
results_by_alid = {}
allprops = am.getmapping().keys()
for r in aq.execute(allprops,[]):
    asserter('gaia_id' in r)
    # use airport id to map
    results_by_alid[r['al_id']] = r
asserteq(len(results_by_alid), 3)
alaska_dict = d
aq.commit()

compareobj(alaska_dict, results_by_alid[idnum], am)
compareobj(a2d, results_by_alid[a2id], am)
compareobj(a3d, results_by_alid[a3id], am)

# delete by gaia_id, object 2
aq.begin()
aq.delete(results_by_alid[a2id]['gaia_id'])
asserteq(len(getelements()), 2)
results = []
for res in aq.execute(allprops, []):
    results.append(res)
aq.commit()
asserteq(len(results), 2)
asserter((results[0]['al_id'] in  [idnum, a3id]) and
         (results[1]['al_id'] in  [idnum, a3id]) and
         (results[1]['al_id'] != results[0]['al_id']))

aq.begin()
# test inserting with invalid property; a3d otherwise okay
a3d['city'] = "nowhereman"
assert_failed_as_expected = True
try:
    aq.insert(a3d)
    assert_failed_as_expected = False
except:
    aq.rollback()
    pass
asserter(assert_failed_as_expected)

