#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

from db_mapper.route_mapping import RouteMapper
from db_mapper.route_query import RouteQuery
from db_mapper.airport_mapping import AirportMapper
from db_mapper.airport_query import AirportQuery
import flatbuffers
from db_mapper.AirportDemo import routes

from test_aids import asserter, asserteq, compareobj, make_airport, make_airport2, make_airport3

# Mapping test, Query test:
#  This test writes to the 'real' gaia cow store.
#  It uses Airports for nodes and Routes for edges.
#
# Mapping test:
# create a route fb
# convert to dictionary
# convert dictionary to fb
# compare props

b = flatbuffers.Builder(0)

# gaiaid = 0xcafebeef. not setting id in the fb
al_id = 666 # Oceanic Airlines
src_ap_id = 999
dst_ap_id = 42
rawsrc_airport = "Sydney"
rawdst_airport = "Los Angeles"
rawcodeshare  = ""
stops = 5
rawequipment = "777"
src_airport = b.CreateString(rawsrc_airport)
dst_airport = b.CreateString(rawdst_airport)
codeshare = b.CreateString(rawcodeshare)
equipment = b.CreateString(rawequipment)

# create serialized form as bytes
routes.routesStart(b)
#routes.routesAddGaiaN(b, gaiaid) # not setting gaiaid (nested type)
routes.routesAddAlId(b, al_id)
routes.routesAddSrcAp(b, src_airport)
routes.routesAddSrcApId(b, src_ap_id)
routes.routesAddDstAp(b, dst_airport)
routes.routesAddDstApId(b, dst_ap_id)
routes.routesAddCodeshare(b, codeshare)
routes.routesAddStops(b, stops)
routes.routesAddEquipment(b, equipment)
a = routes.routesEnd(b)
b.Finish(a)

data = b.Bytes
pos = b.Head()

# init fb with it
rm = RouteMapper()
oceanic_airlines = rm.BytestoFB(data, pos)
#asserteq(gaiaid, oceanic_airlines.GaiaN()) not using id for now
asserter(al_id == oceanic_airlines.AlId())
asserter(rawsrc_airport == oceanic_airlines.SrcAp())
asserter(src_ap_id == oceanic_airlines.SrcApId())
asserter(rawdst_airport == oceanic_airlines.DstAp())
asserter(dst_ap_id == oceanic_airlines.DstApId())
asserter(rawcodeshare == oceanic_airlines.Codeshare())
asserteq(stops, oceanic_airlines.Stops())
asserter(rawequipment == oceanic_airlines.Equipment())

d = rm.FBtoDictionary(oceanic_airlines)
asserter(al_id == d["al_id"])
asserter(rawsrc_airport == d["src_ap"])
asserter(src_ap_id == d["src_ap_id"])
asserter(rawdst_airport == d["dst_ap"])
asserter(dst_ap_id == d["dst_ap_id"])
asserter(rawcodeshare == d["codeshare"])
asserter(stops == d["stops"])
asserter(rawequipment == d["equipment"])
asserteq(len(d), 12)

# round trip it - already did dictionary to bytes, bytes to fb, fb to dictionary
# now that dictionary to bytes to flat buffer.
serialized_d_bytes = rm.DictionarytoBytes(d)
oceanic_roundtrip = rm.BytestoFB(serialized_d_bytes.Bytes, serialized_d_bytes.Head())
#asserter(oceanic_airlines.GaiaE() == oceanic_roundtrip.GaiaE())
asserter(oceanic_airlines.AlId() == oceanic_roundtrip.AlId())
asserter(oceanic_airlines.SrcAp() == oceanic_roundtrip.SrcAp())
asserter(oceanic_airlines.SrcApId() == oceanic_roundtrip.SrcApId())
asserter(oceanic_airlines.DstAp() == oceanic_roundtrip.DstAp())
asserter(oceanic_airlines.DstApId() == oceanic_roundtrip.DstApId())
asserter(oceanic_airlines.Codeshare() == oceanic_roundtrip.Codeshare())
asserter(oceanic_airlines.Stops() == oceanic_roundtrip.Stops())
asserter(oceanic_airlines.Equipment() == oceanic_roundtrip.Equipment())

# Test query, also test the real store.
# It uses the AirportQuery and RouteQuery but both should use the same COW storage.
#
# insert 1 row, with also 2 airports
# do query
# check data
# insert another route that reverses the first.
# query for all, check they are there
# delete the second one.
# check it's gone via query
#
# d is oceanic_airlines in dictionary form

# Insert 2 airports using AirportQuery, which are really different elements in the gaia cow store, its the same store as the rq uses.
def getelements(q):
    res = []
    for blah in q.execute([],[]):
        res.append(blah)
    return res
rq = RouteQuery({}, {})
rq.begin()
asserteq(len(getelements(rq)), 0)
rq.commit()
# Insert 2 airports using AirportQuery, which are really different elements in the gaia cow store, its the same store as the rq uses.
aq = AirportQuery({}, {})
aq.begin()
asserteq(len(getelements(aq)), 0)
aq.commit()

a2 = make_airport2()
a3 = make_airport3()
a2gaiaid = 2 << 35
a3gaiaid = a2gaiaid+1
assert(a2gaiaid != a3gaiaid)

a2['gaia_id'] = a2gaiaid
a3['gaia_id'] = a3gaiaid
# insert the route, with valid ids to the 2 ends of the edge
d['gaia_src_id'] = a2gaiaid
d['gaia_dst_id'] = a3gaiaid
aq.begin()
aq.insert(a2)
aq.insert(a3)
asserteq(len(getelements(aq)), 2)
aq.commit()

# Can't insert the route (edge, which is d) until the nodes are inserted (a2, a3)
rq.begin()
result = rq.insert(d)
id = result['gaia_id']
for edge in rq.execute([],[]):
    if id == edge['gaia_id']:
        found = True
        break
rq.commit()
asserter(found)
assert(id == edge['gaia_id'])
assert(id not in [a2gaiaid, a3gaiaid])
assert(isinstance(id, (int, long)))

# check for the two airports, the nodes
airports = {}
aq.begin()
for val in aq.execute([],[]):
    airports[val['gaia_id']] = val
aq.commit()
asserteq(len(airports), 2)
assert(a2gaiaid in airports)
assert(a3gaiaid in airports)
# make sure all props are there
# can't compare until we actually query
compareobj(d, edge, rm)

# make another route, in the reverse direction of the first route
r2 = {}
for p in rm.getmapping().keys():
    r2[p] = None

r2al2id = 123456
r2['al_id'] = r2al2id
r2['src_ap_id'] = d['dst_ap_id'] # swapped airports
r2['dst_ap_id'] = d['src_ap_id']
r2['gaia_src_id'] = d['gaia_dst_id']
r2['gaia_dst_id'] = d['gaia_src_id']

rq.begin()
r2result = rq.insert(r2)
rq.commit()
r2id = r2result['gaia_id']

# requery
routes = {}
airports = {}
aq.begin()
for a in aq.execute([],[]):
    airports[a['gaia_id']] = a
for r in rq.execute([],[]):
    routes[r['gaia_id']] = r

aq.commit()
asserteq(len(routes), 2)
asserteq(len(airports), 2)
asserter(r2id not in airports)
asserter(r2id in routes)
asserter(r2id != id)
r2id = r2result['gaia_id']

# verify all 4 keys unique
idset = set()
for aid in airports:
    idset.add(aid)
for rid in routes:
    idset.add(rid)
asserteq(len(idset), 4)

compareobj(d, routes[id], rm)
am = AirportMapper()
compareobj(a2, airports[a2gaiaid], am)
compareobj(a3, airports[a3gaiaid], am)

# delete the first route
# id is the first route
rq.begin()
rq.delete(id)
rq.commit() # have to commit before looking at live values
rq.begin()
# verify only routes are the second one.
for r2old in rq.execute([],[]):
    asserteq(r2old['gaia_id'], r2id)
rq.commit()

# TODO check for inserting invalid routes with src or dst that doesn't exist.

# test valid update of a route
asserter(r2old['equipment'] is None)
r2new = r2old.copy()
r2new['equipment'] = '747'
rq.begin()
rq.update(r2old, r2new)
# get the single result
for r2changed in rq.execute([],[]):
    asserteq(r2changed['gaia_id'], r2id)
    asserteq(r2changed['equipment'], '747')
rq.commit()
# test invalid update.
badid = 999999
asserter(r2changed['gaia_id'] != badid)
r2v2 = r2changed.copy()
r2v2['gaia_id'] = badid

# test changing the gaia_id, which is not supported
update_didnt_succeed = True
try:
    rq.update(r2changed, r2v2)
    update_didnt_succeed = False  # update should throw and not get here
except:
    pass
asserter(update_didnt_succeed)

# Now test updating an invalid object, badid obj doesn't exist, it has a different id
r2changed['gaia_id'] = badid
badupdate_failed = True
try:
    rq.update(r2changed, r2v2) # update (old,  tonew)
    badupdate_failed = False # update through throw and not get here
except:
    pass
asserter(badupdate_failed)

