#pragma once

/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <thing_IoTData.h>

#include <chrono>

#include <IoTDataThing.hpp>
#include <JSonThingAPI.hpp>
#include <ThingAPIException.hpp>
#include <iostream>
#include <thread>

#include "I_edge_notified.hpp"
#include "data_listener.hpp"

using namespace std;
using namespace com::adlinktech::datariver;
using namespace com::adlinktech::iot;

/**
* @brief UI Thing
*/

class ui_thing : I_edge_notified
{

private:
    string m_thingPropertiesUri;
    DataRiver m_dataRiver;
    Thing m_thing = create_thing();
    data_listener m_dataListener;
    message::i_message_bus* m_callbackClass;

    // the name of the client on the message bus
    const std::string m_sender_name_c = "UI";

    // message header data
    int m_sequence_ID = 0;
    int m_sender_ID = 0;
    std::string m_sender_name = m_sender_name_c;
    int m_dest_ID = 0;
    std::string m_dest_name = "*";

    /**
    * Create the Edge thing
    *
    * @return Thing
    * @throws ThingAPIException
    * @exceptsafe yes
    */
    Thing create_thing()
    {

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
    /**
    * Constructor
    *
    * @param[in] DataRiver dataRiver
    * @param[in] message::i_message_bus* callbackClass
    * @param[in] string thingPropertiesUri
    * @throws ThingAPIException
    * @exceptsafe yes
    */
    ui_thing(DataRiver data_river, message::i_message_bus* callback_class, string thing_properties_uri)
        : m_thingPropertiesUri(thing_properties_uri), m_dataRiver(data_river), m_callbackClass(callback_class)
    {

        m_dataListener.register_listener(this);
        m_thing.addListener(m_dataListener);

        //cout << "UI Thing started" << endl;
    }

    /**
    * Destructor
    *
    * @exceptsafe yes
    */
    ~ui_thing() = default;
    //{
        //m_dataRiver.close();
        //cout << "UI Thing stopped" << endl;
    //}

    /**
     * Put a message on the bus
     *
     * @param[in] std::shared_ptr<bus_messages::message> msg
     * @return int
     * @throws 
     * @exceptsafe yes
     */
    int send_message(std::shared_ptr<bus_messages::message> msg)
    {

        auto message_type = msg->get_message_type_name();

        if (message_type == bus_messages::message_types::action_message)
        {

            auto action_message = reinterpret_cast<bus_messages::action_message*>(msg.get());

            IOT_VALUE actor_type_v;
            IOT_VALUE actor_v;
            IOT_VALUE action_v;
            IOT_VALUE arg1_v;

            actor_type_v.iotv_string(action_message->m_actor_type);
            actor_v.iotv_string(action_message->m_actor);
            action_v.iotv_string(action_message->m_action);
            actor_type_v.iotv_string(action_message->m_arg1);

            IOT_NVP_SEQ action_data = {
                IOT_NVP(string("actorType"), actor_type_v),
                IOT_NVP(string("actor"), actor_v),
                IOT_NVP(string("action"), action_v),
                IOT_NVP(string("arg1"), arg1_v)};

            m_thing.write("action", action_data);
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
    void notify_data_available(const std::vector<DataSample<IOT_NVP_SEQ>>& dataIn)
    {

        //TODO : determine what kind of message we have here

        //cout << "Got Data" << endl;

        if (nullptr == m_callbackClass)
            return;

        // extract data values
        for (auto data : dataIn)
        {
            std::string title = "";
            std::string body = "";
            uint32_t severity = 0;
            std::string arg1 = "";

            auto dat = data.getData();

            for (auto dv : dat)
            {
                auto name = dv.name();
                auto value = dv.value();

                auto type = dv.value_._d();

                // check if we got nuthin
                if (type == com::adlinktech::iot::IOT_TYPE::TYPE_NONE)
                    continue;

                if (name == "title")
                {
                    title = value.iotv_string();
                }
                else if (name == "body")
                {
                    body = value.iotv_string();
                }
                else if (name == "severity")
                {
                    severity = value.iotv_uint32();
                }
                else if (name == "arg1")
                {
                    arg1 = value.iotv_string();
                }
            }

            // create message payload
            bus_messages::message_header mh(m_sequence_ID++, m_sender_ID, m_sender_name, m_dest_ID, m_dest_name);

            std::shared_ptr<bus_messages::message> msg = std::make_shared<bus_messages::alert_message>(mh, title, body, severity, arg1);

            // notify internal bus of new message
            m_callbackClass->message_received_from_bus(msg);
        }
    }
};
