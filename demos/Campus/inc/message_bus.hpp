#pragma once

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
typedef void (*MessageCallbackType)(message::Message msg); 

namespace message {

/**
* @brief The message bus interface
*/
class IMessageBus
{
public:
  
    std::vector<MessageCallbackType> _messageCallbacks;

    // send a message
    virtual int SendMessage(Message msg)
    {
        UNUSED(msg);
        return 0;
    }

    // register a callback on which to receive messages
    virtual int RegisterMessageCallback(MessageCallbackType callback) 
    {
        _messageCallbacks.push_back(callback);
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

    std::queue<Message> _messageQueue;
    std::mutex m;
    std::condition_variable cv;

    /**
     * Worker thread, runs forever
     *
     * @param[in] 
     * @param[out] 
     * @return 
     * @throws 
     * @exceptsafe yes
     */    
    void worker()
    {
        while(true)
        {
            // Wait until a message is received
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk);

            // empty the queue
            while(!_messageQueue.empty())
            {
                // get the first message
                Message message = _messageQueue.front();
                _messageQueue.pop();

                // send message to each registerd callback
                for(MessageCallbackType cb : _messageCallbacks)
                {
                    cb(message);

                    // TODO: Don't send messages back to sender (maybe)
                    // TODO: Add error handling
                }
            }
        }

    }

public:

    /**
     * constructor
     *
     * @param[in] 
     * @param[out] 
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
     * @param[in] Message message
     * @param[out] 
     * @return 
     * @throws 
     * @exceptsafe yes
     */  
    int SendMessage(Message message) override
    {
        {
            std::lock_guard<std::mutex> lk(m);
            _messageQueue.push(message);
        }

        cv.notify_one();
        return 0;
    }

    /**
     * Start the worker thread
     *
     * @param[in] 
     * @param[out] 
     * @return 
     * @throws 
     * @exceptsafe yes
     */  
    int Run()
    {
        std::thread workerThread(&MessageBusInProc::worker, this);
        return 0;
    }

};

} // namespace message