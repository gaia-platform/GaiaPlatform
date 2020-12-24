#pragma once

/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <thing_IoTData.h>

#include <IoTDataThing.hpp>
#include <JSonThingAPI.hpp>
#include <ThingAPIException.hpp>

#include "../../common/inc/message_bus.hpp"

#include "campus_thing.hpp"
#include "person_thing.hpp"
#include "ui_thing.hpp"

using namespace std;
using namespace com::adlinktech::datariver;
using namespace com::adlinktech::iot;

namespace message
{
/**
 * @brief An ADLink Edge message bus
 */
class message_bus : public i_message_bus
{
private:
    DataRiver m_dataRiver;
    std::shared_ptr<ui_thing> m_uiThing = nullptr;
    std::shared_ptr<person_thing> m_personThing = nullptr;
    std::shared_ptr<campus_thing> m_campusThing = nullptr;

    /**
     * Log message to stdout
     * 
     * @param[in] std::string prefix e
     * @param[in] const std::exception& e
     * @return void
     * @throws 
     * @exceptsafe yes
     */
    void log_this(std::string prefix, const std::exception& e)
    {
        std::cout << "Exception: " << prefix << " : " << e.what();
    }

    /**
     * Log message to stdout
     * 
     * @param[in] std::string prefix e
     * @return void
     * @throws 
     * @exceptsafe yes
     */
    void log_this(std::string prefix)
    {
        std::cout << prefix;
    }

    /**
     * Check if message sender is the callback registrant
     * 
     * @param[in] std::shared_ptr<bus_messages::message> msg
     * @param[in] callback_registration registration
     * @return true if the same, false if not the same
     * @throws std::invalid_argument
     * @exceptsafe yes
     */
    bool is_registrant_sender(std::shared_ptr<bus_messages::message> msg, callback_registration registration)
    {
        if (nullptr == msg)
            throw std::invalid_argument("argument msg cannot be null");

        return 0 == registration.get_registrant_name().compare(msg->get_sender_name()) ? true : false;
    }

public:
    /**
     * constructor
     *
     * @return 
     * @throws 
     * @exceptsafe yes
     */
    message_bus()
    {
        init();
    }

    /**
     * Initialize the Edge SDK DataRiver
     * 
     * These env vars must be set prior to execution
     * 
     * ADLINK_DATARIVER_URI = "file:///media/mark/Data1/U20/SDK/ADLINK/EdgeSDK/1.5.1/etc/config/default_datariver_config_v1.6.xml"
     * ADLINK_LICENSE = file:///media/mark/Data1/U20/SDK/ADLINK/EdgeSDK/1.5.1/etc/license.lic"   
     * 
     * @return 
     * @throws 
     * @exceptsafe yes
     */
    bool init()
    {
        try
        {
            m_dataRiver = com::adlinktech::datariver::DataRiver::getInstance();
            m_uiThing = std::make_shared<ui_thing>(m_dataRiver, this, "file://./config/UIProperties.json");
            m_personThing = std::make_shared<person_thing>(m_dataRiver, this, "file://./config/PersonProperties.json");
            m_campusThing = std::make_shared<campus_thing>(m_dataRiver, this, "file://./config/CampusProperties.json");
        }
        catch (ThingAPIException& e)
        {
            std::cerr << "Exception in MessageBus.init() : " << e.what() << endl;
            throw;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Exception in MessageBus.init() : " << e.what() << '\n';
            throw;
        }
        catch (...)
        {
            std::cerr << "Exception in MessageBus.init() : ..." << '\n';
            throw;
        }

        return true;
    }

    /**
     * Put a message on the bus
     *
     * @param[in] std::shared_ptr<bus_messages::message> msg
     * @return int
     * @throws 
     * @exceptsafe yes
     */
    int send_message(std::shared_ptr<bus_messages::message> msg) override
    {
        {
            try
            {
                auto hdr = msg->get_header();

                // TODO : This is a hacky way to determine which thing sent
                // the message. Ok for now, change it soon though.
                if (hdr.get_senderName() == "termUi")
                {
                    m_uiThing->send_message(msg);
                }
                else if (hdr.get_senderName() == "campus")
                {
                    m_campusThing->send_message(msg);
                }
                else if (hdr.get_senderName() == "person")
                {
                    m_personThing->send_message(msg);
                }
            }
            catch (ThingAPIException& e)
            {
                std::cerr << "Exception in MessageBus.init() : " << e.what() << endl;
                throw;
            }
            catch (const std::exception& e)
            {
                std::cerr << "Exception in MessageBus.init() : " << e.what() << '\n';
                throw;
            }
            catch (...)
            {
                std::cerr << "Exception in MessageBus.init() : ..." << '\n';
                throw;
            }
        }

        return 0;
    }

    /**
     * Message received from the message bus
     *
     * @param[in] std::shared_ptr<bus_messages::message> msg
     * @return int
     * @throws 
     * @exceptsafe yes
     */
    int message_received_from_bus(std::shared_ptr<bus_messages::message> msg) override
    {
        {
            try
            {
                if (nullptr == msg)
                {
                    log_this("messge == null");
                    return -1;
                }

                // send message to each registered callback
                for (callback_registration cbr : m_message_callbacks)
                {
                    try
                    {
                        // Don't echo messages back to sender
                        if (!is_registrant_sender(msg, cbr))
                            cbr.get_callback()(msg);
                    }
                    catch (const std::exception& e)
                    {
                        log_this("Processing callback", e);
                    }
                }
            }
            catch (ThingAPIException& e)
            {
                std::cerr << "Exception in MessageBus.init() : " << e.what() << endl;
                throw;
            }
            catch (const std::exception& e)
            {
                std::cerr << "Exception in MessageBus.init() : " << e.what() << '\n';
                throw;
            }
            catch (...)
            {
                std::cerr << "Exception in MessageBus.init() : ..." << '\n';
                throw;
            }
        }

        return 0;
    }
}; // class message_bus
} // namespace message