#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# functions to help testing
from db_mapper.airport_mapping import AirportMapper
am = AirportMapper()
def asserter(check):
    if not check:
        raise AssertionError("failed test %s" % check)

def asserteq(act, exp):
    if not (act == exp):
        raise AssertionError("failed test: actual %s expected %s" % (act,exp))

def compareobj(a, resobj, m):
    """ resobject is after inserting, has the gaia_id added """
    # inserted data has the id column added when data is returned
    asserteq(len(resobj), len(m.getmapping()))

    for p in m.getmapping().keys():
        if p == "gaia_id":
            continue
        asserteq(a[p], resobj[p])

a1id = 72476
a1rawname = "Walnut Ridge Regional Airport"
a1rawcity = "Walnut Ridge"
a2id = 72207
a2rawname = "Little Rock International"
a2rawcity = "Little Rock"
a3id = 12345
a3rawname = "Conway Airport"
a3rawcity = "Conway, AR"

def initairportfields(d):
    for p in am.getmapping().keys():
        d[p] = None

def make_airport(id, name, city):
    a = {}
    initairportfields(a)
    a['ap_id'] = id
    a['name'] = name
    a['city'] = city
    a['dst'] = "0"
    a['country'] = "USA"
    a['altitude'] = 0
    a['iata'] = "Hel"
    a['icao'] = "Hell"
    a['longitude'] = 95
    a['latitude'] = 27
    a['timezone'] = 8
    a['type'] = "bad"
    a['tztext'] = "tuba"
    a['source'] = "none"
    return a

def make_airport2():
    return make_airport(a2id, a2rawname, a2rawcity)
def make_airport3():
    return make_airport(a3id, a3rawname, a3rawcity)

