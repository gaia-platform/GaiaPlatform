#pragma once

#include <time.h>
#include <string>

namespace message {

/**
* The message header
*/
class MessageHeader{
public:

    unsigned int _sequenceID = 0;
    time_t _timeSent;
    unsigned int _senderID;
    std::string _senderName;    
    unsigned int _destID;
    std::string _destName;

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
        _timeSent  = time(nullptr);
        
        _sequenceID++;
        _senderID = 0;
        _senderName = "*";    
        _destID = 0;
        _destName = "*";
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
        _sequenceID(sequenceID), _senderID(senderID), 
        _senderName(senderName), _destID(destID), _destName(destName)
    {
        _timeSent  = time(nullptr);
        FixVals();
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
        _sequenceID(sequenceID), _timeSent(timeSent), _senderID(senderID), 
        _senderName(senderName), _destID(destID), _destName(destName)
    {
        FixVals();
    };

    int DemoTest(){
        return 0;
    }

    void FixVals()
    {
        if( "" == _senderName ) 
         _senderName = "*";

        if( "" == _destName) 
          _destName = "*";
    }
};

/**
* The message 
*/
/*template <class T>
class Message{

private:

    MessageHeader messageHeader_;    
    T payload_;
    
public:

    Message(MessageHeader header, T payload) : 
        messageHeader_(header), payload_(payload){
    }

    Message(){
    }

    int DemoTest(){
        return 0;
    }
};*/


/**
* The message 
*/
class Message{

protected:

    MessageHeader _messageHeader;    
    void * _payload;
    
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
    template <class T>    
    Message(MessageHeader header, T payload) : 
        _messageHeader(header)
    {
        _payload = payload;
    }

    /**
     * Constructor
     *
     * @param[in] MessageHeader header
     * @return 
     * @throws 
     * @exceptsafe yes
     */    
    Message(MessageHeader header) : 
        _messageHeader(header)
    {}

    Message(){}

    int DemoTest(){
        return 0;
    }
};

class ActionMessage : public Message
{
public:

    std::string _actorType;
    std::string _actor;
    std::string _action;
    std::string _arg1;

    ActionMessage(MessageHeader header, std::string actorType, std::string actor, std::string action, std::string arg1) :
        Message(header), _actorType(actorType), _actor(actor), _action(action), _arg1(arg1)
    {}

    ActionMessage()
    {}

    int DemoTest(){
        return 0;
    }
};

}