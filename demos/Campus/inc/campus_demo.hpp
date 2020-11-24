#pragma once
/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <memory>
#include <iostream>
#include "message.hpp" 
#include "message_bus.hpp"
#include "gaia_system.hpp"
#include "rules.hpp"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

#include "../generated/gaia_campus.h"
#include "../inc/icampus.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::rules;
using namespace std;

namespace CampusDemo{
class Campus : ICampus, std::enable_shared_from_this<Campus> {

private:

    //Run the rule trigger fake
    bool _rule_trigger_fake = false; 

    //std::thread* _workerThread;

    //sleep time of worker in ms
    useconds_t _sleepTime = 10;

    //TODO: this is a hack to store persons to work around a thread issue
    //std::vector<gaia::campus::person_t> _persons_v;

    // the name of the client on the message bus
    const std::string _sender_name = "campus";    

    // message header data
    int _sequenceID = 0;
    int _senderID = 0;
    std::string _senderName = _sender_name;
    int _destID = 0;
    std::string _destName = "*";

    // the config file
    const std::string _config_file_name = "./src/gaia_conf.toml"; //TODO: Is that path ok?

    // singletonish
    inline static Campus* _lastInstance = nullptr;

    // the message bus we want to use
    std::shared_ptr<message::IMessageBus> _messageBus = nullptr;

    static Campus* GetLastInstance();

        /**
     * Log message to stdout
     * 
     * @param[in] std::string prefix e
     * @param[in] const std::exception& e
     * @return void
     * @throws 
     * @exceptsafe yes
     */      
    void log_this(std::string prefix, const std::exception& e);

    /**
     * Log message to stdout
     * 
     * @param[in] std::string prefix e
     * @return void
     * @throws 
     * @exceptsafe yes
     */      
    void log_this(std::string prefix);

    /**
    * Worker for thread which must be kept alive for gaia::init
    *
    * @return void
    * @throws 
    * @exceptsafe yes
    */  
    void Worker();

    /**
    * Callback from the message bus when a message arrives
    *
    * @param[in] message::Message msg
    * @return 
    * @throws 
    * @exceptsafe yes
    */  
    void MessageCallback(std::shared_ptr<message::Message> msg);

    /**
    * Callback from the message bus when a message arrives
    * Am not crazy about this, I need to convert to using a std::fuction
    *
    * @param[in] message::Message msg
    * @return 
    * @throws 
    * @exceptsafe yes
    */  
    static void StaticMessageCallback(std::shared_ptr<message::Message> msg);

    //********************************

    void got_person_action_message(const message::ActionMessage *msg);

    bool get_person(const char* name, gaia::campus::person_t &found_person);
    gaia_id_t insert_campus(const char* name, bool in_emergency);
    void update_campus(gaia::campus::campus_t& camp, bool in_emergency);
    gaia_id_t insert_person(const char* name, bool is_threat, const std::string location);
    void update_person(gaia::campus::person_t& person, bool is_threat, const std::string location);
    void restore_default_values(); 
    void init_storage();

    //*** ICampus interface *****************************

    void cb_action( std::string actorType, std::string actorName, 
        std::string actionName, std::string arg1) override;
    void cb_alert( std::string title, std::string body, 
        int severity, std::string arg1) override;

public:

    Campus();

    ~Campus();

        /**
     * Run the campus demo.
     *
     * @param[in] std::shared_ptr<message::IMessageBus> messageBus
     * @return 
     * @throws
     * @exceptsafe basic?
     */
    int Init(std::shared_ptr<message::IMessageBus> messageBus);

    /**
     * Run the campus demo.
     *
     * @param[out,in] 
     * @return 
     * @throws
     * @exceptsafe basic?
     */
    int RunAsync();

    int DemoTest();

}; // class Campus
} // namespace CampusDemo
