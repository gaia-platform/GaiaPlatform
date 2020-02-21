#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

from mapping import FBMapper
import flatbuffers
import AirportDemo.airlines as aa

class AirlineMapper(FBMapper):
    def __init__(self):
        super(AirlineMapper, self).__init__()
        self.SetupMapping()

    # defines the mapping between the relational column names and fb functions
    def SetupMapping(self):
        m = self.mapping = {}
        # can look up by db col name; only type used is str
        m["gaia_id"] = [aa.airlines.GaiaId, aa.airlinesAddGaiaId, "int64"]
        m["al_id"] = [aa.airlines.AlId, aa.airlinesAddAlId, "int32"]
        m["name"]  = [aa.airlines.Name, aa.airlinesAddName, "str"]
        m["alias"] = [aa.airlines.Alias, aa.airlinesAddAlias, "str"]
        m["iata"]  = [aa.airlines.Iata, aa.airlinesAddIata, "str"]
        m["icao"]  = [aa.airlines.Icao, aa.airlinesAddIcao, "str"]
        m["callsign"]  = [aa.airlines.Callsign, aa.airlinesAddCallsign, "str"]
        m["country"]  = [aa.airlines.Country, aa.airlinesAddCountry, "str"]
        m["active"]  = [aa.airlines.Active, aa.airlinesAddActive, "str"]
        # can I just drop aa.airports. => aa.
        self.fbGetRoot = aa.airlines.GetRootAsairlines
        self.fbStart = aa.airlinesStart
        # fix aa.airports
        self.fbEnd = aa.airlinesEnd

