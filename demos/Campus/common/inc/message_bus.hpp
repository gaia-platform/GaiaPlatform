#pragma once

#include <iostream>
#include <stdexcept>
#include <time.h>
#include <string>
#include "message.hpp"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

// to supress unused-parameter build warnings
#define UNUSED(...) (void)(__VA_ARGS__)
    
// callback method type
typedef void (*message_callback_type)(std::shared_ptr<bus_messages::message> msg); 

namespace message {

/*class IMessageCallback
{
    virtual void MessageCallback(std::shared_ptr<bus_messages::message> msg)
    {
        UNUSED(msg);
    }
};*/

/**
* @brief A container to hold callback registrations
*/
class callback_registration{

private:

    message_callback_type _callback = nullptr;
    std::string _regsitrantName = "";

public:

    message_callback_type get_callback(){
        return _callback;
    }

    std::string get_registrant_name(){
        return _regsitrantName;
    }   

    callback_registration(message_callback_type callback, std::string regsitrantName) :
        _callback(callback), _regsitrantName(regsitrantName){}
};

/**
* @brief The message bus interface
*/
class i_message_bus
{
public:
  
    //std::vector<message_callback_type> _messageCallbacks;
    std::vector<callback_registration> _messageCallbacks;

    // send a message to the message bus
    virtual int send_message(std::shared_ptr<bus_messages::message> msg)
    {
        UNUSED(msg);
        return 0;
    }

    // message received from the message bus
    virtual int message_received_from_bus(std::shared_ptr<bus_messages::message> msg)
    {
        UNUSED(msg);
        return 0;
    }

    // register a callback on which to receive messages
    virtual int register_message_callback(message_callback_type callback, std::string regsitrantName) 
    {
        _messageCallbacks.push_back(callback_registration(callback, regsitrantName));
        return 0;
    }
    
    // register a callback on which to receive messages
    virtual int deregister_message_callback(message_callback_type callback) 
    {
        UNUSED(callback);
        return 0;
    }

    // to get rid of annoying build warnings
    virtual ~i_message_bus(){}
    
    int demo_test(){
        return 0;
    }
};
} // namespace message