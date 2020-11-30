#pragma once

/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <thread>
#include <chrono>

#include <IoTDataThing.hpp>
#include <JSonThingAPI.hpp>
#include <thing_IoTData.h>
#include <ThingAPIException.hpp>

#include "INotified.hpp"
#include "data_listener.hpp"

using namespace std;
using namespace com::adlinktech::datariver;
using namespace com::adlinktech::iot;

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

#define SAMPLE_DELAY_MS 100

/**
* @brief UI Thing
*/

class ui_thing : INotified {

private:

    string m_thingPropertiesUri;
    DataRiver m_dataRiver;
    Thing m_thing = createThing();    
    data_listener m_dataListener;

    Thing createThing(){

        //TODO: it appears that creating a unique tag group more than once does not result in an error
        // 1) verify this, 2) find a way to check if a tag group exists

        // Create and Populate the TagGroup registry with JSON resource files.
        JSonTagGroupRegistry tgr1;
        tgr1.registerTagGroupsFromURI("file://definitions/TagGroup/io.gaiaplatform.campus/ActionTagGroup.json");
        m_dataRiver.addTagGroupRegistry(tgr1);

        // Create and Populate the TagGroup registry with JSON resource files.
        JSonTagGroupRegistry tgr2;
        tgr2.registerTagGroupsFromURI("file://definitions/TagGroup/io.gaiaplatform.campus/AlertTagGroup.json");
        m_dataRiver.addTagGroupRegistry(tgr2);

        // Create and Populate the ThingClass registry with JSON resource files.
        JSonThingClassRegistry tcr;
        tcr.registerThingClassesFromURI("file://definitions/ThingClass/io.gaiaplatform.campus/UIThingClass.json");
        m_dataRiver.addThingClassRegistry(tcr);

        // Create a Thing based on properties specified in a JSON resource file.
        JSonThingProperties tp;
        tp.readPropertiesFromURI(m_thingPropertiesUri);
        return m_dataRiver.createThing(tp);            
    }

public:

    ui_thing(DataRiver dataRiver, string thingPropertiesUri) 
        :m_thingPropertiesUri(thingPropertiesUri), m_dataRiver(dataRiver) {

        m_dataListener.register_listener(this);
        m_thing.addListener(m_dataListener);

        cout << "UI Thing started" << endl;
    }

    ~ui_thing() {
        m_dataRiver.close();
        cout << "UI Thing stopped" << endl;
    }

    /**
     * Put a message on the bus
     *
     * @param[in] std::shared_ptr<message::Message> msg
     * @return int
     * @throws 
     * @exceptsafe yes
     */  
    int SendMessage(std::shared_ptr<message::Message> msg){

        auto messageType = msg->get_message_type_name();
        //char buffer[1024];

        if(messageType == message::message_types::action_message){

            auto actionMessage = reinterpret_cast<message::ActionMessage*>(msg.get());   

            //sprintf(buffer, "Change detected: %s %s %s", actionMessage->_actor.c_str(), 
            //    actionMessage->_action.c_str(), actionMessage->_arg1.c_str());

            IOT_VALUE actorType_v;            
            IOT_VALUE actor_v;            
            IOT_VALUE action_v;            
            IOT_VALUE arg1_v;

            actorType_v.iotv_string(actionMessage->_actorType);            
            actor_v.iotv_string(actionMessage->_actor);            
            action_v.iotv_string(actionMessage->_action);            
            actorType_v.iotv_string(actionMessage->_arg1);

            IOT_NVP_SEQ actionData = {
                IOT_NVP(string("actorType"), actorType_v),                
                IOT_NVP(string("actor"), actor_v),                
                IOT_NVP(string("action"), action_v),                
                IOT_NVP(string("arg1"), arg1_v)
            };            

            m_thing.write("action", actionData);
        }
        else
            return -1;
        
        {
            //std::lock_guard<std::mutex> lk(m);
            //_messageQueue.push(msg);
        }

        //cv.notify_one();
        return 0;
    }

    /**
     * Receive a message from the bus
     *
     * @param[in] std::shared_ptr<message::Message> msg
     * @return int
     * @throws 
     * @exceptsafe yes
     */  
    void notify_data_available(const std::vector<DataSample<IOT_NVP_SEQ> >& dataIn) {
       
       //TODO : determine what kind of message we have here

       // translate

       // send to reistrants

        cout << "Got Data" << endl;

        for( auto data : dataIn) 
        {
            auto dat = data.getData();

            for(auto dv : dat)
            {
                auto n = dv.name();
                auto v = dv.value().iotv_string();

                auto bb = dv.name();
            }
        }
    }
};

