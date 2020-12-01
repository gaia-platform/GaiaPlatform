#pragma once

#include <time.h>
#include <string>

namespace bus_messages {

/**
* The message header
*/
class message_header{
private:

    unsigned int _sequenceID = 0;
    time_t _timeSent;
    unsigned int _senderID;
    std::string _senderName;    
    unsigned int _destID;
    std::string _destName;

public:

    /**
     * Constructor, timeSent is set for you
     *
     * @param[in] 
     * @return 
     * @throws 
     * @exceptsafe yes
     */
    message_header()
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
    message_header(unsigned int sequenceID,
        unsigned int senderID,
        std::string senderName,    
        unsigned int destID,
        std::string destName) : 
        _sequenceID(sequenceID), _senderID(senderID), 
        _senderName(senderName), _destID(destID), _destName(destName)
    {
        _timeSent  = time(nullptr);
        fix_vals();
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
    message_header(unsigned int sequenceID,
        time_t timeSent,
        unsigned int senderID,
        std::string senderName,    
        unsigned int destID,
        std::string destName) : 
        _sequenceID(sequenceID), _timeSent(timeSent), _senderID(senderID), 
        _senderName(senderName), _destID(destID), _destName(destName)
    {
        fix_vals();
    };

    int demo_test(){
        return 0;
    }

    void fix_vals()
    {
        if( "" == _senderName ) 
         _senderName = "*";

        if( "" == _destName) 
          _destName = "*";
    }

    unsigned int get_sequenceID(){
        return _sequenceID;
    }

    time_t get_timeSent(){
        return _timeSent;
    }
    
    unsigned int get_senderID(){
        return _senderID;
    }

    std::string get_senderName(){
        return _senderName;
    }    

    unsigned int get_destID(){
        return _destID;
    }

    std::string get_destName(){
        return _destName;
    }
};

/**
* The message 
*/
/*template <class T>
class Message{

private:

    message_header messageHeader_;    
    T payload_;
    
public:

    Message(message_header header, T payload) : 
        messageHeader_(header), payload_(payload){
    }

    Message(){
    }

    int DemoTest(){
        return 0;
    }
};*/


/**
* The message. The base class of all messages. 
*/
class message{

protected:

    message_header _messageHeader;    
    void * _payload;
    std::string _messageTypeName = "";
    
public:

    /**
     * Constructor
     *
     * @param[in] message_header header
     * @param[in] T payload
     * @return 
     * @throws 
     * @exceptsafe yes
     */    
    template <class T>    
    message(message_header header, T payload) : 
        _messageHeader(header)
    {
        _payload = payload;
    }

    /**
     * Constructor
     *
     * @param[in] message_header header
     * @return 
     * @throws 
     * @exceptsafe yes
     */    
    message(message_header header) : 
        _messageHeader(header)
    {}

    message(){}

    message_header get_header(){
        return _messageHeader;
    }

    std::string get_sender_name(){
        return _messageHeader.get_senderName();
    }

    std::string get_message_type_name(){
        return _messageTypeName;
    }

    int demo_test(){
        return 0;
    }
};

/*
*   A container of a list of message type names. We use this instead of an enum
*   so that we can identify message types across process and machine boundaries.
*   This can be improved, but we keep in simple in this simple example.
*/
class message_types
{
public:
    
    inline static const std::string action_message = "message.action_message";
    inline static const std::string alert_message = "message.alert_message";
};

/*
*   The action message, used to convey that some object has taken some action,
*   like a move to a new loocation or a change in role. 
*/
class action_message : public message
{
private:

    std::string _typeName = message_types::action_message;

public:

    std::string _actorType;
    std::string _actor;
    std::string _action;
    std::string _arg1;

    action_message(message_header header, std::string actorType, std::string actor, std::string action, std::string arg1) :
        message(header), _actorType(actorType), _actor(actor), _action(action), _arg1(arg1){ 
        _messageTypeName = message_types::action_message;
    }

    action_message(){ 
        _messageTypeName = message_types::action_message;
    }

    int demo_test(){
        return 0;
    }
};

/*
*   The alert message message, used to send an alert 
*/
class alert_message : public message
{
private:

    std::string _typeName = message_types::alert_message;

public:

    enum severity_level_enum {notice, alert, emergency};

    std::string _title;
    std::string _body;
    severity_level_enum _severity;
    std::string _arg1;

    alert_message(message_header header, std::string title, std::string body, severity_level_enum severity, std::string arg1) :
        message(header), _title(title), _body(body), _severity(severity), _arg1(arg1){ 
        _messageTypeName = message_types::alert_message;
    }

    alert_message(message_header header, std::string title, std::string body, int severity, std::string arg1) :
        message(header), _title(title), _body(body), _arg1(arg1){ 
        _severity = static_cast<severity_level_enum>(severity); //TODO : catch arg problems
        _messageTypeName = message_types::alert_message;
    }

    alert_message(){ 
        _messageTypeName = message_types::alert_message;
    }

    int demo_test(){
        return 0;
    }
};

} // bus_messages