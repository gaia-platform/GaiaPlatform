#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

from mapping import FBMapper
import flatbuffers
import AirportDemo.routes as aa

class RouteMapper(FBMapper):
    def __init__(self):
        super(RouteMapper, self).__init__()
        self.SetupMapping()

    # defines the mapping between the relational column names and fb functions
    def SetupMapping(self):
        m = self.mapping = {}
        # can look up by db col name; only type used is str
        m["gaia_id"] = [aa.routes.GaiaId, aa.routesAddGaiaId, "int64"]
        m["gaia_src_id"] = [aa.routes.GaiaSrcId, aa.routesAddGaiaSrcId, "int64"]
        m["gaia_dst_id"] = [aa.routes.GaiaDstId, aa.routesAddGaiaDstId, "int64"]
        m["airline"] = [aa.routes.Airline, aa.routesAddAirline, "str"]
        m["al_id"] = [aa.routes.AlId, aa.routesAddAlId, "int32"]
        m["src_ap"]  = [aa.routes.SrcAp, aa.routesAddSrcAp, "str"]
        m["src_ap_id"] = [aa.routes.SrcApId, aa.routesAddSrcApId, "int32"]
        m["dst_ap"]  = [aa.routes.DstAp, aa.routesAddDstAp, "str"]
        m["dst_ap_id"]  = [aa.routes.DstApId, aa.routesAddDstApId, "int32"]
        m["codeshare"]  = [aa.routes.Codeshare, aa.routesAddCodeshare, "str"]
        m["stops"]  = [aa.routes.Stops, aa.routesAddStops, "int32"]
        m["equipment"]  = [aa.routes.Equipment, aa.routesAddEquipment, "str"]
        # can I just trop aa.routes. => aa.
        self.fbGetRoot = aa.routes.GetRootAsroutes
        self.fbStart = aa.routesStart
        # fix aa.routes
        self.fbEnd = aa.routesEnd

