#pragma once

#include <time.h>
#include <string>

namespace message {

/**
* The message header
*/
class MessageHeader{
public:

    unsigned int sequenceID_;
    time_t timeSent_;
    unsigned int senderID_;
    std::string senderName_;    
    unsigned int destID_;
    std::string destName_;

    /**
     * Constructor, timeSent is set for you
     *
     * @param[in] 
     * @return 
     * @throws 
     * @exceptsafe yes
     */
    MessageHeader()
    {
        timeSent_  = time(nullptr);
    };

    /**
     * Constructor, timeSent is set for you
     *
     * @param[in] unsigned int sequenceID
     * @param[in] unsigned int senderID
     * @param[in] std::string senderName
     * @param[in] unsigned int destID
     * @param[in] std::string destName
     * @return 
     * @throws 
     * @exceptsafe yes
     */
    MessageHeader(unsigned int sequenceID,
    unsigned int senderID,
    std::string senderName,    
    unsigned int destID,
    std::string destName) : 
        sequenceID_(sequenceID), senderID_(senderID), 
        senderName_(senderName), destID_(destID), destName_(destName)
    {
        timeSent_  = time(nullptr);
    };

    /**
     * Constructor, you supply your own timeSent
     *
     * @param[in] unsigned int sequenceID
     * @param[in] time_t timeSent
     * @param[in] unsigned int senderID
     * @param[in] std::string senderName
     * @param[in] unsigned int destID
     * @param[in] std::string destName
     * @return 
     * @throws 
     * @exceptsafe yes
     */    
    MessageHeader(unsigned int sequenceID,
    time_t timeSent,
    unsigned int senderID,
    std::string senderName,    
    unsigned int destID,
    std::string destName) : 
        sequenceID_(sequenceID), timeSent_(timeSent), senderID_(senderID), 
        senderName_(senderName), destID_(destID), destName_(destName)
    {
    };

    int DemoTest(){
        return 0;
    }
};

/**
* The message 
*/
template <class T>
class Message{

private:

    MessageHeader messageHeader_;    
    T payload_;
    
public:

    /**
     * Constructor
     *
     * @param[in] MessageHeader header
     * @param[in] T payload
     * @return 
     * @throws 
     * @exceptsafe yes
     */    
    Message(MessageHeader header, T payload) : 
        messageHeader_(header), payload_(payload){
    }

    Message(){
    }

    int DemoTest(){
        return 0;
    }
};

}