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
typedef void (*MessageCallbackType)(std::shared_ptr<message::Message> msg); 

namespace message {

/*class IMessageCallback
{
    virtual void MessageCallback(std::shared_ptr<message::Message> msg)
    {
        UNUSED(msg);
    }
};*/

/**
* @brief A container to hold callback registrations
*/
class callback_registration{

private:

    MessageCallbackType _callback = nullptr;
    std::string _regsitrantName = "";

public:

    MessageCallbackType get_callback(){
        return _callback;
    }

    std::string get_registrant_name(){
        return _regsitrantName;
    }   

    callback_registration(MessageCallbackType callback, std::string regsitrantName) :
        _callback(callback), _regsitrantName(regsitrantName){}
};

/**
* @brief The message bus interface
*/
class IMessageBus
{
public:
  
    //std::vector<MessageCallbackType> _messageCallbacks;
    std::vector<callback_registration> _messageCallbacks;

    // send a message to the message bus
    virtual int SendMessage(std::shared_ptr<message::Message> msg)
    {
        UNUSED(msg);
        return 0;
    }

    // message received from the message bus
    virtual int message_received_from_bus(std::shared_ptr<message::Message> msg)
    {
        UNUSED(msg);
        return 0;
    }

    // register a callback on which to receive messages
    virtual int RegisterMessageCallback(MessageCallbackType callback, std::string regsitrantName) 
    {
        _messageCallbacks.push_back(callback_registration(callback, regsitrantName));
        return 0;
    }
    
    // register a callback on which to receive messages
    virtual int DeRegisterMessageCallback(MessageCallbackType callback) 
    {
        UNUSED(callback);
        return 0;
    }

    // to get rid of annoying build warnings
    virtual ~IMessageBus(){}
    
    int DemoTest(){
        return 0;
    }
};
} // namespace message