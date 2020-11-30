#pragma once

/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "../../common/inc/message_bus.hpp"
#include "ui_thing.hpp"

#include <IoTDataThing.hpp>
#include <JSonThingAPI.hpp>
#include <thing_IoTData.h>
#include <ThingAPIException.hpp>

using namespace std;
using namespace com::adlinktech::datariver;
using namespace com::adlinktech::iot;

namespace message {
/**
 * @brief An ADLink Edge message bus
 */
class MessageBus : public IMessageBus
{
private:

    DataRiver m_dataRiver;
    std::shared_ptr<ui_thing> m_uiThing = nullptr;

    std::thread* _workerThread;
    std::queue<std::shared_ptr<message::Message>> _messageQueue;
    std::mutex m;
    std::condition_variable cv;
    bool _stop = false;

    /**
     * Log message to stdout
     * 
     * @param[in] std::string prefix e
     * @param[in] const std::exception& e
     * @return void
     * @throws 
     * @exceptsafe yes
     */      
    void log_this(std::string prefix, const std::exception& e){
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
    void log_this(std::string prefix){
        std::cout << prefix;
    }

    /**
     * Check if message sender is the callback registrant
     * 
     * @param[in] std::shared_ptr<message::Message> msg
     * @param[in] callback_registration registration
     * @return true if the same, false if not the same
     * @throws std::invalid_argument
     * @exceptsafe yes
     */  
    bool is_registrant_sender(std::shared_ptr<message::Message> msg, 
        callback_registration registration)
    {
        if(nullptr == msg)
            throw std::invalid_argument("argument msg cannot be null");

        //auto registrant_name = registration.get_registrant_name();
        //auto sender_name = msg->get_sender_name();

        return 0 == registration.get_registrant_name().compare(msg->get_sender_name()) ? true : false;    
    }

    /**
     * Worker thread, runs forever
     *
     * @return 
     * @throws 
     * @exceptsafe yes
     */    
    void worker()
    {
        while(!_stop)
        {
            // Wait until a message is received
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk);

            if(_stop)
                break;

            // empty the queue
            while(!_messageQueue.empty())
            {
                // get the first message
                std::shared_ptr<message::Message> msg = _messageQueue.front();
                _messageQueue.pop();

                if(nullptr == msg)
                {
                    log_this("messge == null");
                    continue;
                }

                // send message to each registered callback
                for(callback_registration cbr : _messageCallbacks)
                {
                    try {
                        // Don't echo messages back to sender
                        if(!is_registrant_sender( msg, cbr))
                            cbr.get_callback()(msg);
                    } catch(const std::exception& e) {
                        log_this("Processing callback", e);
                    }
                }
            }
        }
    }

public:

    /**
     * constructor
     *
     * @return 
     * @throws 
     * @exceptsafe yes
     */  
    MessageBus()
    {
        init();
        Run();
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
    bool init(){   
        try
        {
            m_dataRiver = com::adlinktech::datariver::DataRiver::getInstance();
            m_uiThing = std::make_shared<ui_thing>(m_dataRiver, "file://./config/UIProperties.json");            
        }
        catch (ThingAPIException& e) 
        {
            std::cerr << "Exception in MessageBus.init() : " << e.what() << endl;
            throw;
        }
        catch(const std::exception& e)
        {
            std::cerr << "Exception in MessageBus.init() : " << e.what() << '\n';
            throw;
        }        
        catch(...)
        {
            std::cerr << "Exception in MessageBus.init() : ..." << '\n';
            throw;
        }
        
        return true;
    }

    /**
     * Put a message on the bus
     *
     * @param[in] std::shared_ptr<message::Message> msg
     * @return int
     * @throws 
     * @exceptsafe yes
     */  
    int SendMessage(std::shared_ptr<message::Message> msg) override
    {
        {
            try
            {
                auto hdr = msg->get_header();

                // TODO : This is a hacky way to determine which thing sent
                // the message. Ok for now, change it soon though.
                if(hdr.get_senderName() == "termUi")
                {
                    m_uiThing->SendMessage(msg);
                }

                //std::lock_guard<std::mutex> lk(m);
                _messageQueue.push(msg);                
            }
            catch (ThingAPIException& e) 
            {
                std::cerr << "Exception in MessageBus.init() : " << e.what() << endl;
                throw;
            }
            catch(const std::exception& e)
            {
                std::cerr << "Exception in MessageBus.init() : " << e.what() << '\n';
                throw;
            }        
            catch(...)
            {
                std::cerr << "Exception in MessageBus.init() : ..." << '\n';
                throw;
            }
        }

        cv.notify_one();
        return 0;
    }

    /**
     * Start the worker thread
     *
     * @return int
     * @throws 
     * @exceptsafe yes
     */  
    int Run()
    {
        _workerThread = new std::thread(&MessageBus::worker, this);        
        return 0;
    }

    /**
     * Stop the worker thread
     *
     * @throws 
     * @exceptsafe yes
     */  
    void Stop()
    {
        _stop = true;

        {
            std::lock_guard<std::mutex> lk(m);
        }

        cv.notify_one();
    }

};

} // namespace message