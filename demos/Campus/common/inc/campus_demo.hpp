#pragma once
/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <memory>
#include <iostream>
#include "message.hpp" 
#include "message_bus.hpp"
#include "system.hpp"
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
#include "../inc/i_campus.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::rules;
using namespace std;

namespace campus_demo{
class campus : i_Campus, std::enable_shared_from_this<campus> {

private:

    //Run the rule trigger fake
    bool m_rule_trigger_fake = false; 

    //std::thread* _workerThread;

    //sleep time of worker in ms
    useconds_t m_sleepTime = 10;

    //TODO: this is a hack to store persons to work around a thread issue
    //std::vector<gaia::campus::person_t> _persons_v;

    // the name of the client on the message bus
    const std::string m_sender_name = "campus";    

    // message header data
    int m_sequenceID = 0;
    int m_senderID = 0;
    std::string m_senderName = m_sender_name;
    int m_destID = 0;
    std::string m_destName = "*";

    // the config file
    const std::string m_config_file_name = "./gaia_conf.toml"; //TODO: Is that path ok?
    
    // singletonish
    inline static campus* m_lastInstance = nullptr;

    // the message bus we want to use
    std::shared_ptr<message::i_message_bus> m_messageBus = nullptr;

    static campus* get_last_instance();

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
    void worker();

    /**
    * Callback from the message bus when a message arrives
    *
    * @param[in] bus_messages::message msg
    * @return 
    * @throws 
    * @exceptsafe yes
    */  
    void message_callback(std::shared_ptr<bus_messages::message> msg);

    /**
    * Callback from the message bus when a message arrives
    * Am not crazy about this, I need to convert to using a std::fuction
    *
    * @param[in] bus_messages::message msg
    * @return 
    * @throws 
    * @exceptsafe yes
    */  
    static void static_message_callback(std::shared_ptr<bus_messages::message> msg);

    //********************************

    void got_person_action_message(const bus_messages::action_message *msg);

    bool get_person(const char* name, gaia::campus::person_t &found_person);
    bool get_event(const char* name, gaia::campus::Events_t &found_event);
    gaia_id_t insert_campus(const char* name, bool in_emergency);
    void update_campus(gaia::campus::campus_t& camp, bool in_emergency);
    gaia_id_t insert_person(const char* name, bool is_threat, const std::string location);
    void update_person(gaia::campus::person_t& person, bool is_threat, const std::string location);
    void insert_event_registration(gaia::campus::person_t& person, std::string event_name);

    void restore_default_values(); 
    void init_storage();

    //*** i_Campus interface *****************************

    void cb_action( std::string actorType, std::string actorName, 
        std::string actionName, std::string arg1) override;
    void cb_alert( std::string title, std::string body, 
        int severity, std::string arg1) override;

    enum person_action_enum {move_to = 0, change_role = 1, regsiter_for_event = 2, brandish_weapon = 3, disarm = 4 };
    std::string m_person_action[5] = {"Move To", "Change Role", "Register For Event", "Brandish Weapon", "Disarm"};

    std::string m_actor_type[2] = {"Person", "Car"};

    std::string m_person_roles[4] = {"Stranger", "Student", "Teacher", "Parent"};

    std::string m_cars[3] = {"Unidentified", "Ford Fairlane", "Purple Lambo"};

    std::string m_car_action[1] = {"Move To"};

    std::string m_person_locations[3] = {"Front Door", "Lab", "Main Hall"};

    std::string m_car_locations[3] = {"Entry", "Garage", "Concourse"};

public:

    campus();

    ~campus();

        /**
     * Run the campus demo.
     *
     * @param[in] std::shared_ptr<message::i_message_bus> messageBus
     * @return 
     * @throws
     * @exceptsafe basic?
     */
    int init(std::shared_ptr<message::i_message_bus> messageBus);

    /**
     * Run the campus demo.
     *
     * @param[out,in] 
     * @return 
     * @throws
     * @exceptsafe basic?
     */
    int run_async();

    int demo_test();

    std::vector<std::string> get_actor_type_list();

    std::vector<std::string> get_person_role_list();

    std::vector<std::string> get_event_name_list();

    std::vector<std::string> get_person_name_list();

    std::vector<std::string> get_person_action_list();

    std::vector<std::string> get_person_location_list();

    std::vector<std::string> get_car_list();

    std::vector<std::string> get_car_action_list();

    std::vector<std::string> get_car_location_list();

}; // class campus
} // namespace campus_demo
