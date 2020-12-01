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

class campus_thing : I_edge_notified {

private:

    string m_thingPropertiesUri;
    DataRiver m_dataRiver;
    Thing m_thing = createThing();    
    data_listener m_dataListener;
    message::IMessageBus* m_callbackClass;

    // the name of the client on the message bus
    const std::string _sender_name = "campus";

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
        tcr.registerThingClassesFromURI("file://definitions/ThingClass/io.gaiaplatform.campus/CampusThingClass.json");
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
    * @param[in] message::IMessageBus* callbackClass
    * @param[in] string thingPropertiesUri
    * @throws ThingAPIException
    * @exceptsafe yes
    */  
    campus_thing(DataRiver dataRiver, message::IMessageBus* callbackClass, string thingPropertiesUri) 
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
    ~campus_thing() {
        //m_dataRiver.close();
        //cout << "Person Thing stopped" << endl;
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

        if(messageType == message::message_types::alert_message){

            auto actionMessage = reinterpret_cast<message::alert_message*>(msg.get());   

            IOT_VALUE title_v;            
            IOT_VALUE body_v;            
            IOT_VALUE severity_v;            
            IOT_VALUE arg1_v;

            title_v.iotv_string(actionMessage->_title);            
            body_v.iotv_string(actionMessage->_body);            
            severity_v.iotv_uint32(actionMessage->_severity);            
            arg1_v.iotv_string(actionMessage->_arg1);

            IOT_NVP_SEQ alertData = {
                IOT_NVP(string("title"), title_v),                
                IOT_NVP(string("body"), body_v),                
                IOT_NVP(string("severity"), severity_v),                
                IOT_NVP(string("arg1"), arg1_v)
            };            

            m_thing.write("alert", alertData);
        }
        else
            return -1;
        
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
            message::MessageHeader mh(_sequenceID++, _senderID, _senderName, _destID, _destName);

            std::shared_ptr<message::Message> msg = 
                std::make_shared<message::ActionMessage>(mh, actorType, actorName, actionName, arg1);

            // notify internal bus of new message
            m_callbackClass->message_received_from_bus(msg);
        }
    }
};

