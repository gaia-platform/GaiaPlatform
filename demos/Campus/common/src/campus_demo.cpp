/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "../inc/campus_demo.hpp"

// to supress unused-parameter build warnings
#define UNUSED(...) (void)(__VA_ARGS__)

campus_demo::campus::campus(){
    m_lastInstance = this;
}

campus_demo::campus::~campus(){}

int campus_demo::campus::demo_test(){
    return 0;
}

campus_demo::campus* campus_demo::campus::get_last_instance(){
    return m_lastInstance;
}

void campus_demo::campus::log_this(std::string prefix, const std::exception& e){
    auto ew = e.what();
    UNUSED(ew);
    std::cout << "Exception: " << prefix << " : " << e.what();
}

void campus_demo::campus::log_this(std::string prefix){
    std::cout << prefix;
}

void campus_demo::campus::got_person_action_message(const bus_messages::action_message *msg){
    
    gaia::campus::person_t found_person;

    //if we can't find them in the DB then exit. 
    //obviously this is not how we would proceed in production, an unidentified thing may be a security issue
    if( !get_person(msg->m_actor.c_str(), found_person))
    {
        //TODO: log message
        return;
    }

    //what kind of action?
    if(msg->m_action == "Brandish Weapon")
    {
        begin_transaction();
        //update that person as a threat
        update_person(found_person,true,found_person.location());
        commit_transaction();

        //rule trigger fake, bypasses rules, for development only
        if(m_rule_trigger_fake){

            if(nullptr == m_messageBus)
                return;

            bus_messages::message_header mh(m_sequenceID++, m_senderID, m_senderName, m_destID, m_destName);

            char buffer[256];
            sprintf(buffer, "'%s' is brandishing a weapon", msg->m_actor.c_str());

            std::shared_ptr<bus_messages::message> msg = 
                std::make_shared<bus_messages::alert_message>(mh, "Deadly Threat", 
                    buffer, bus_messages::alert_message::severity_level_enum::emergency, "");

            m_messageBus->send_message(msg);
        }
    }    
    else if(msg->m_action == "Disarm")
    {
        begin_transaction();
        //update that person as a threat
        update_person(found_person,false,found_person.location());
        commit_transaction();
    }
}

void campus_demo::campus::message_callback(std::shared_ptr<bus_messages::message> msg){
    
    bool gotSession = false;

    try
    {
        // what kind of message do we have?
        auto messageType = msg->get_message_type_name();

        if(messageType == bus_messages::message_types::action_message)
        {
            // begin a Gaia DB session
            begin_session();   
            gotSession = true;         
        
            auto action_message = reinterpret_cast<bus_messages::action_message*>(msg.get());   

            //at this point we have our action message, update the DB
            if(action_message->m_actorType == "Person")
                got_person_action_message(action_message);
        }
    }
    catch(const std::exception& e){
        log_this("campus_demo::campus::MessageCallback()", e);
    }
    catch(...){
        log_this("Exception in campus_demo::campus::MessageCallback() ...");
    }

    if(gotSession){
        // end the Gaia DB session
        end_session();
    }
}

void campus_demo::campus::static_message_callback(std::shared_ptr<bus_messages::message> msg){
    
    auto li = get_last_instance();

    if(nullptr == li) //TODO: notify user
        return;

    li->message_callback(msg);        
}

int campus_demo::campus::run_async(){
    // start a new thread and ...

    return 0;
}

void campus_demo::campus::worker(){
    //Initialize Gaia
    gaia::system::initialize(m_config_file_name.c_str());     

    init_storage();

    //dump_db();

    while(true){
        usleep(m_sleepTime);
    }
}

int campus_demo::campus::init(std::shared_ptr<message::i_message_bus> messageBus){

    try{
        //Disregard all this for now
        //campus_ruleset_p_campus = std::shared_ptr<i_Campus>(this);
        //auto bbb = std::shared_ptr<campus_demo::Campus>(this);    
        //campus_ruleset_p_campus = std::shared_ptr<i_Campus>(reinterpret_cast<i_Campus*>(this))
        //campus_ruleset_p_campus = std::shared_ptr<i_Campus>(reinterpret_cast<i_Campus*>(this));
        //campus_ruleset_p_campus = reinterpret_cast<std::shared_ptr<i_Campus>>(shared_from_this());

        //TODO : yes, I know, make this modern
        campus_ruleset_p_campus = reinterpret_cast<i_Campus*>(this);

        if(nullptr == messageBus)
            throw std::invalid_argument("argument messageBus cannot be null");
        
        //Save the message bus and register a callback
        m_messageBus = messageBus;
        m_messageBus->register_message_callback(&campus_demo::campus::static_message_callback, m_sender_name);

        //Initialize Gaia
        gaia::system::initialize(m_config_file_name.c_str());     

        init_storage();    
    }
    catch(const std::exception& e){
        log_this("campus_demo::campus::Init()", e);
        return -1;
    }
    catch(...){
        log_this("Exception in campus_demo::campus::Init() ...");
        return -1;
    }
    
    return 0;
}

//*** i_Campus interface ***

void campus_demo::campus::cb_action( std::string actorType, 
    std::string actorName, std::string actionName, std::string arg1){

    if(nullptr == m_messageBus)
        return;

    bus_messages::message_header mh(m_sequenceID++, m_senderID, m_senderName, m_destID, m_destName);

    std::shared_ptr<bus_messages::message> msg = 
        std::make_shared<bus_messages::action_message>(mh, actorType, actorName, actionName, arg1);

    m_messageBus->send_message(msg);
}

void campus_demo::campus::cb_alert( std::string title, 
        std::string body, int severity, std::string arg1){

    if(nullptr == m_messageBus)
        return;

    bus_messages::message_header mh(m_sequenceID++, m_senderID, m_senderName, m_destID, m_destName);

    std::shared_ptr<bus_messages::message> msg = 
        std::make_shared<bus_messages::alert_message>(mh, title, body, severity, arg1);

    m_messageBus->send_message(msg);
}

//*** DB Procedural ***

bool campus_demo::campus::get_person(const char* name, gaia::campus::person_t &found_person)
{
    bool did_find = false;

    begin_transaction();
    for (auto& person : gaia::campus::person_t::list())
    {
        if (strcmp(person.name(), name) == 0) {
            found_person = person;
            did_find = true;
            break;
        }
    }
    commit_transaction();
    return did_find;
}

gaia_id_t campus_demo::campus::insert_campus(const char* name, bool in_emergency) {
    gaia::campus::campus_writer cw;
    cw.name = name;
    cw.in_emergency = in_emergency;
    UNUSED(in_emergency);
    return cw.insert_row();
}

void campus_demo::campus::update_campus(gaia::campus::campus_t& camp, bool in_emergency) {
    auto c = camp.writer();
    c.in_emergency = in_emergency;
    c.update_row();
}

gaia_id_t campus_demo::campus::insert_person(const char* name, bool is_threat, const std::string location) {
    gaia::campus::person_writer p;
    p.name = name;
    p.is_threat = is_threat;
    p.location = location;
    return p.insert_row();
}

void campus_demo::campus::update_person(gaia::campus::person_t& person, bool is_threat, const std::string location) {
    auto p = person.writer();
    p.is_threat = is_threat;
    p.location = location;
    p.update_row();
}

void campus_demo::campus::restore_default_values() {
    for (auto& person : gaia::campus::person_t::list()) {
        //_persons_v.push_back(person);
        update_person(person, false, "*");
    }
}

void campus_demo::campus::init_storage() {
    gaia::direct_access::auto_transaction_t tx(gaia::direct_access::auto_transaction_t::no_auto_begin);

    if (gaia::campus::person_t::get_first()) {
        restore_default_values();
        tx.commit();
        return;
    }

    // campus

    gaia::campus::campus_t campus = gaia::campus::campus_t::get(insert_campus("School 17", false));

    // buildings

    auto bldg = gaia::campus::Buildings_t::insert_row("B1", "Shasta");
    campus.BuildingsCampus_Buildings_list().insert(bldg);  
    auto bldg_shasta = gaia::campus::Buildings_t::get(bldg);     
    
    bldg = gaia::campus::Buildings_t::insert_row("B2", "Everest");
    campus.BuildingsCampus_Buildings_list().insert(bldg);    
    auto bldg_everest = gaia::campus::Buildings_t::get(bldg);   

    bldg = gaia::campus::Buildings_t::insert_row("B3", "Garage");
    campus.BuildingsCampus_Buildings_list().insert(bldg);   
    auto bldg_garage = gaia::campus::Buildings_t::get(bldg);    

    bldg = gaia::campus::Buildings_t::insert_row("B4", "Security");
    campus.BuildingsCampus_Buildings_list().insert(bldg);
    auto bldg_security = gaia::campus::Buildings_t::get(bldg);   

    // rooms

    auto room = gaia::campus::Rooms_t::insert_row("26", "Eagle", "2", 50);
    bldg_shasta.RoomsBuildings_Rooms_list().insert(room);
    auto room_eagle = gaia::campus::Rooms_t::get(room);

    room = gaia::campus::Rooms_t::insert_row("1", "Crow", "1", 100);
    bldg_shasta.RoomsBuildings_Rooms_list().insert(room);
    auto room_crow = gaia::campus::Rooms_t::get(room);

    // persons

    auto person = gaia::campus::person_t::insert_row("Unidentified", 0, "*");
    campus.PersonCampus_person_list().insert(person);
    auto person_unidentified = gaia::campus::person_t::get(person);

    person = gaia::campus::person_t::insert_row("Bob Kabob", 0, "*");
    campus.PersonCampus_person_list().insert(person);
    auto person_bob = gaia::campus::person_t::get(person);

    person = gaia::campus::person_t::insert_row("Sam Kabam", 0, "*");
    campus.PersonCampus_person_list().insert(person);
    auto person_sam = gaia::campus::person_t::get(person);

    // staff

    auto staff = gaia::campus::Staff_t::insert_row("S1", "4/4/1044");
    auto staff_bob = gaia::campus::Staff_t::get(staff);
    staff_bob.PersonsStaff_Persons_list().insert(person_bob.gaia_id());

    // events

    // event 1

    auto evnt = gaia::campus::Events_t::insert_row("E1", "Orientation", "5/10/2020", "16:00", "18:00", 0);
    campus.EventsCampus_Events_list().insert(evnt);
    auto evnt_orientation = gaia::campus::Events_t::get(evnt);

    evnt_orientation.RoomEvents_Rooms_list().insert(room_eagle);
    evnt_orientation.StaffEvents_Staff_list().insert(staff_bob);

    // event 2

    evnt = gaia::campus::Events_t::insert_row("E2", "Prom", "5/11/2020", "16:00", "18:00", 0);
    campus.EventsCampus_Events_list().insert(evnt);
    auto evnt_prom = gaia::campus::Events_t::get(evnt);

    evnt_orientation.RoomEvents_Rooms_list().insert(room_crow);
    evnt_orientation.StaffEvents_Staff_list().insert(staff_bob);

    tx.commit();
}