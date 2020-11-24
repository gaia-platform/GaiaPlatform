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

    // send a message
    virtual int SendMessage(std::shared_ptr<message::Message> msg)
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

/**
 * @brief A local in process message bus
 */
class MessageBusInProc : public IMessageBus
{
private:

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
    MessageBusInProc()
    {
        Run();
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
            //std::lock_guard<std::mutex> lk(m);
            _messageQueue.push(msg);
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
        _workerThread = new std::thread(&MessageBusInProc::worker, this);        
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