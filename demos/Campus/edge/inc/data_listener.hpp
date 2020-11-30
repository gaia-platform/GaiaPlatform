#pragma once

/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <IoTDataThing.hpp>
#include <JSonThingAPI.hpp>
#include <thing_IoTData.h>
#include <ThingAPIException.hpp>

#include "INotified.hpp"

using namespace std;
using namespace com::adlinktech::datariver;
using namespace com::adlinktech::iot;

/**
* @brief Implements an Edge SDK message bus listener
*/

class data_listener : public DataAvailableListener<IOT_NVP_SEQ> {

private:

    INotified* m_callback;

    /**
     * Called by the Data River wher a message has arrived from the message bus
     * @param[in] const std::vector<com::adlinktech::datariver::DataSample<com::adlinktech::iot::IOT_NVP_SEQ>>& dataIn
     * @return void
     * @throws 
     * @exceptsafe yes
     */       
    void notifyDataAvailable(const std::vector<DataSample<IOT_NVP_SEQ> >& data) {
       
        if(nullptr == m_callback)
            return;
            
        m_callback->notify_data_available(data);
    }

public:

    /**
     * Regsiter a callback class to be informed when a message arrives on the bus
     * @param[in] INotified* caller
     * @return void
     * @throws 
     * @exceptsafe yes
     */       
    void register_listener(INotified* caller){
        m_callback = caller;
    }
};

