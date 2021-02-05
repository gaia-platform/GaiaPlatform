#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

from mapping import FBMapper
import flatbuffers
import AirportDemo.airports as aa

class AirportMapper(FBMapper):
    def __init__(self):
        super(AirportMapper, self).__init__()
        self.SetupMapping()

    # defines the mapping between the relational column names and fb functions
    def SetupMapping(self):
        m = self.mapping = {}
        # can look up by db col name; only type used is str
        m["gaia_id"] = [aa.airports.GaiaId, aa.airportsAddGaiaId, "int64"]
        m["ap_id"] = [aa.airports.ApId, aa.airportsAddApId, "int32"]
        m["name"]  = [aa.airports.Name, aa.airportsAddName, "str"]
        m["city"]  = [aa.airports.City, aa.airportsAddCity, "str"]
        m["country"]  = [aa.airports.Country, aa.airportsAddCountry, "str"]
        m["iata"]  = [aa.airports.Iata, aa.airportsAddIata, "str"]
        m["icao"]  = [aa.airports.Icao, aa.airportsAddIcao, "str"]
        m["latitude"]  = [aa.airports.Latitude, aa.airportsAddLatitude, "double"]
        m["longitude"]  = [aa.airports.Longitude, aa.airportsAddLongitude, "double"]
        m["altitude"] = [aa.airports.Altitude, aa.airportsAddAltitude, "int32"]
        m["timezone"]  = [aa.airports.Timezone, aa.airportsAddTimezone, "float"]
        m["dst"]  = [aa.airports.Dst, aa.airportsAddDst, "str"]
        m["tztext"]  = [aa.airports.Tztext, aa.airportsAddTztext, "str"]
        m["type"]  = [aa.airports.Type, aa.airportsAddType, "str"]
        m["source"]  = [aa.airports.Source, aa.airportsAddSource, "str"]
        # can I just trop aa.airports. => aa.
        self.fbGetRoot = aa.airports.GetRootAsairports
        self.fbStart = aa.airportsStart
        # fix aa.airports
        self.fbEnd = aa.airportsEnd

