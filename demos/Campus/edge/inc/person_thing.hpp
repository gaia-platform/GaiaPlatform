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

#include "../../common/inc/message_bus.hpp"
#include "I_edge_notified.hpp"
#include "data_listener.hpp"

using namespace std;
using namespace com::adlinktech::datariver;
using namespace com::adlinktech::iot;

/**
* @brief UI Thing
*/

class person_thing : I_edge_notified {

private:

    string m_thingPropertiesUri;
    DataRiver m_dataRiver;
    Thing m_thing = create_thing();    
    data_listener m_dataListener;
    message::i_message_bus* m_callbackClass;

    // the name of the client on the message bus
    const std::string _sender_name = "Person";

    // message header data
    int _sequenceID = 0;
    int _senderID = 0;
    std::string _senderName = _sender_name;
    int _destID = 0;
    std::string _destName = "*";

    /**
     * Create the Edge thing
     *
     * @return Thing
     * @throws ThingAPIException
     * @exceptsafe yes
     */  
    Thing create_thing(){

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
        tcr.registerThingClassesFromURI("file://definitions/ThingClass/io.gaiaplatform.campus/PersonThingClass.json");
        m_dataRiver.addThingClassRegistry(tcr);

        // Create a Thing based on properties specified in a JSON resource file.
        JSonThingProperties tp;
        tp.readPropertiesFromURI(m_thingPropertiesUri);
        return m_dataRiver.createThing(tp);            
    }

public:

    /**
    * Constructor
    *
    * @param[in] DataRiver dataRiver
    * @param[in] message::i_message_bus* callbackClass
    * @param[in] string thingPropertiesUri
    * @throws ThingAPIException
    * @exceptsafe yes
    */  
    person_thing(DataRiver dataRiver, message::i_message_bus* callbackClass, string thingPropertiesUri) 
        :m_thingPropertiesUri(thingPropertiesUri), m_dataRiver(dataRiver), m_callbackClass(callbackClass) {

        m_dataListener.register_listener(this);
        m_thing.addListener(m_dataListener);

        //cout << "Person Thing started" << endl;
    }

    /**
    * Destructor
    *
    * @exceptsafe yes
    */     
    ~person_thing() {
        //cout << "Person Thing stopped" << endl;
    }

    /**
    * Put a message on the bus
    *
    * @param[in] std::shared_ptr<bus_messages::message> msg
    * @return int
    * @throws 
    * @exceptsafe yes
    */  
    int send_message(std::shared_ptr<bus_messages::message> msg){

        auto messageType = msg->get_message_type_name();

        if(messageType == bus_messages::message_types::action_message){

            auto action_message = reinterpret_cast<bus_messages::action_message*>(msg.get());   

            IOT_VALUE actorType_v;            
            IOT_VALUE actor_v;            
            IOT_VALUE action_v;            
            IOT_VALUE arg1_v;

            actorType_v.iotv_string(action_message->_actorType);            
            actor_v.iotv_string(action_message->_actor);            
            action_v.iotv_string(action_message->_action);            
            actorType_v.iotv_string(action_message->_arg1);

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
        
        return 0;
    }

    /**
     * Receive a message from the bus
     *
     * @param[in] std::shared_ptr<bus_messages::message> msg
     * @return int
     * @throws 
     * @exceptsafe yes
     */  
    void notify_data_available(const std::vector<DataSample<IOT_NVP_SEQ> >& dataIn) {
       
        //TODO : determine what kind of message we have here

        //cout << "Got Data" << endl;

        if(nullptr == m_callbackClass)
            return;

        // extract data values
        for( auto data : dataIn) 
        {
            std::string actorType = "Person";
            std::string actorName = "";
            std::string actionName = "";
            std::string arg1 = "";

            auto dat = data.getData();

            for(auto dv : dat)
            {
                auto name = dv.name();
                auto value = dv.value();

                auto type = dv.value_._d();

                // check if we got nuthin
                if( type == com::adlinktech::iot::IOT_TYPE::TYPE_NONE)
                    continue;

                if(name == "actorType")
                { actorType = value.iotv_string(); }
                else if(name == "actor")
                {actorName = value.iotv_string();}
                else if(name == "action")
                {actionName = value.iotv_string();}
                else if(name == "arg1")
                {arg1 = value.iotv_string();}
            }

            actorType = "Person";

            // create message payload
            bus_messages::message_header mh(_sequenceID++, _senderID, _senderName, _destID, _destName);

            std::shared_ptr<bus_messages::message> msg = 
                std::make_shared<bus_messages::action_message>(mh, actorType, actorName, actionName, arg1);

            // notify internal bus of new message
            m_callbackClass->message_received_from_bus(msg);
        }
    }
};

