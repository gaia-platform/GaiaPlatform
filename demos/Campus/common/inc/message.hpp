#pragma once

#include <time.h>

#include <string>

namespace bus_messages
{

/**
* The message header
*/
class message_header
{
private:
    unsigned int m_sequenceID = 0;
    time_t m_timeSent;
    unsigned int m_senderID;
    std::string m_senderName;
    unsigned int m_destID;
    std::string m_destName;

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
        m_timeSent = time(nullptr);

        m_sequenceID++;
        m_senderID = 0;
        m_senderName = "*";
        m_destID = 0;
        m_destName = "*";
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
    message_header(unsigned int sequenceID, unsigned int senderID, std::string senderName, unsigned int destID, std::string destName)
        : m_sequenceID(sequenceID), m_senderID(senderID), m_senderName(senderName), m_destID(destID), m_destName(destName)
    {
        m_timeSent = time(nullptr);
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
    message_header(unsigned int sequenceID, time_t timeSent, unsigned int senderID, std::string senderName, unsigned int destID, std::string destName)
        : m_sequenceID(sequenceID), m_timeSent(timeSent), m_senderID(senderID), m_senderName(senderName), m_destID(destID), m_destName(destName)
    {
        fix_vals();
    };

    int demo_test()
    {
        return 0;
    }

    void fix_vals()
    {
        if ("" == m_senderName)
            m_senderName = "*";

        if ("" == m_destName)
            m_destName = "*";
    }

    unsigned int get_sequenceID()
    {
        return m_sequenceID;
    }

    time_t get_timeSent()
    {
        return m_timeSent;
    }

    unsigned int get_senderID()
    {
        return m_senderID;
    }

    std::string get_senderName()
    {
        return m_senderName;
    }

    unsigned int get_destID()
    {
        return m_destID;
    }

    std::string get_destName()
    {
        return m_destName;
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
class message
{

protected:
    message_header m_messageHeader;
    void* m_payload;
    std::string m_messageTypeName = "";

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
    message(message_header header, T payload)
        : m_messageHeader(header)
    {
        m_payload = payload;
    }

    /**
     * Constructor
     *
     * @param[in] message_header header
     * @return 
     * @throws 
     * @exceptsafe yes
     */
    message(message_header header)
        : m_messageHeader(header)
    {
    }

    message()
    {
    }

    message_header get_header()
    {
        return m_messageHeader;
    }

    std::string get_sender_name()
    {
        return m_messageHeader.get_senderName();
    }

    std::string get_message_type_name()
    {
        return m_messageTypeName;
    }

    int demo_test()
    {
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
    std::string m_typeName = message_types::action_message;

public:
    std::string m_actor_type;
    std::string m_actor;
    std::string m_action;
    std::string m_arg1;

    action_message(message_header header, std::string actor_type, std::string actor, std::string action, std::string arg1)
        : message(header), m_actor_type(actor_type), m_actor(actor), m_action(action), m_arg1(arg1)
    {
        m_messageTypeName = message_types::action_message;
    }

    action_message()
    {
        m_messageTypeName = message_types::action_message;
    }

    int demo_test()
    {
        return 0;
    }
};

/*
*   The alert message message, used to send an alert 
*/
class alert_message : public message
{
private:
    std::string m_typeName = message_types::alert_message;

public:
    enum severity_level_enum
    {
        notice,
        alert,
        emergency
    };

    std::string m_title;
    std::string m_body;
    severity_level_enum m_severity;
    std::string m_arg1;

    alert_message(message_header header, std::string title, std::string body, severity_level_enum severity, std::string arg1)
        : message(header), m_title(title), m_body(body), m_severity(severity), m_arg1(arg1)
    {
        m_messageTypeName = message_types::alert_message;
    }

    alert_message(message_header header, std::string title, std::string body, int severity, std::string arg1)
        : message(header), m_title(title), m_body(body), m_arg1(arg1)
    {
        m_severity = static_cast<severity_level_enum>(severity); //TODO : catch arg problems
        m_messageTypeName = message_types::alert_message;
    }

    alert_message()
    {
        m_messageTypeName = message_types::alert_message;
    }

    int demo_test()
    {
        return 0;
    }
};

} // namespace bus_messages