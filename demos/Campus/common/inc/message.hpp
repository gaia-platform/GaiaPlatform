#pragma once

#include <ctime>
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
    message_header(unsigned int sequence_id, unsigned int sender_id, std::string sender_name, unsigned int dest_id, std::string dest_name)
        : m_sequenceID(sequence_id), m_senderID(sender_id), m_senderName(std::move(sender_name)), m_destID(dest_id), m_destName(std::move(dest_name))
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
    message_header(unsigned int sequence_id, time_t time_sent, unsigned int sender_id, std::string sender_name, unsigned int dest_id, std::string dest_name)
        : m_sequenceID(sequence_id), m_timeSent(time_sent), m_senderID(sender_id), m_senderName(std::move(sender_name)), m_destID(dest_id), m_destName(std::move(dest_name))
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

    unsigned int get_sequence_id()
    {
        return m_sequenceID;
    }

    time_t get_time_sent()
    {
        return m_timeSent;
    }

    unsigned int get_sender_id()
    {
        return m_senderID;
    }

    std::string get_sender_name()
    {
        return m_senderName;
    }

    unsigned int get_dest_id()
    {
        return m_destID;
    }

    std::string get_dest_name()
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
    message_header m_message_header;
    void* m_payload;
    std::string m_message_type_name = "";

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
        : m_message_header(std::move(header))
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
    explicit message(message_header header)
        : m_message_header(std::move(header))
    {
    }

    message() = default;
 
    message_header get_header()
    {
        return m_message_header;
    }

    std::string get_sender_name()
    {
        return m_message_header.get_sender_name();
    }

    std::string get_message_type_name()
    {
        return m_message_type_name;
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
        : message(header), m_actor_type(std::move(actor_type)), m_actor(std::move(actor)), m_action(std::move(action)), m_arg1(std::move(arg1))
    {
        m_message_type_name = message_types::action_message;
    }

    action_message()
    {
        m_message_type_name = message_types::action_message;
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
        : message(header), m_title(std::move(title)), m_body(std::move(body)), m_severity(severity), m_arg1(std::move(arg1))
    {
        m_message_type_name = message_types::alert_message;
    }

    alert_message(message_header header, std::string title, std::string body, int severity, std::string arg1)
        : message(header), m_title(std::move(title)), m_body(std::move(body)), m_arg1(std::move(arg1))
    {
        m_severity = static_cast<severity_level_enum>(severity); //TODO : catch arg problems
        m_message_type_name = message_types::alert_message;
    }

    alert_message()
    {
        m_message_type_name = message_types::alert_message;
    }

    int demo_test()
    {
        return 0;
    }
};

} // namespace bus_messages