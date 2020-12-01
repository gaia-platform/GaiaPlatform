#pragma once

/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <IoTDataThing.hpp>
#include <JSonThingAPI.hpp>
#include <thing_IoTData.h>
#include <ThingAPIException.hpp>

/**
* @brief Interface used by classes that provide Edge message callbacks
*/

class I_edge_notified {

public:

    /**
     * A message has arrived from the message bus
     * @param[in] const std::vector<com::adlinktech::datariver::DataSample<com::adlinktech::iot::IOT_NVP_SEQ>>& dataIn
     * @return void
     * @throws 
     * @exceptsafe yes
     */   
    virtual void notify_data_available(
        const std::vector
            <com::adlinktech::datariver::DataSample
                <com::adlinktech::iot::IOT_NVP_SEQ> >& dataIn) = 0;

    /**
     * Destructor, to get rid of annoying build warnings
     * 
     * @throws 
     * @exceptsafe yes
     */   
    virtual ~I_edge_notified(){}
};

