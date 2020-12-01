/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "../inc/campus_demo.hpp"

// to supress unused-parameter build warnings
#define UNUSED(...) (void)(__VA_ARGS__)

CampusDemo::Campus::Campus(){
    _lastInstance = this;
}

CampusDemo::Campus::~Campus(){}

int CampusDemo::Campus::DemoTest(){
    return 0;
}

CampusDemo::Campus* CampusDemo::Campus::GetLastInstance(){
    return _lastInstance;
}

void CampusDemo::Campus::log_this(std::string prefix, const std::exception& e){
    auto ew = e.what();
    UNUSED(ew);
    std::cout << "Exception: " << prefix << " : " << e.what();
}

void CampusDemo::Campus::log_this(std::string prefix){
    std::cout << prefix;
}

void CampusDemo::Campus::got_person_action_message(const message::ActionMessage *msg){
    
    gaia::campus::person_t found_person;

    //if we can't find them in the DB then exit. 
    //obviously this is not how we would proceed in production, an unidentified thing may be a security issue
    if( !get_person(msg->_actor.c_str(), found_person))
    {
        //TODO: log message
        return;
    }

    //what kind of action?
    if(msg->_action == "Brandish Weapon")
    {
        begin_transaction();
        //update that person as a threat
        update_person(found_person,true,found_person.location());
        commit_transaction();

        //rule trigger fake, bypasses rules, for development only
        if(_rule_trigger_fake){

            if(nullptr == _messageBus)
                return;

            message::MessageHeader mh(_sequenceID++, _senderID, _senderName, _destID, _destName);

            char buffer[256];
            sprintf(buffer, "'%s' is brandishing a weapon", msg->_actor.c_str());

            std::shared_ptr<message::Message> msg = 
                std::make_shared<message::alert_message>(mh, "Deadly Threat", 
                    buffer, message::alert_message::severity_level_enum::emergency, "");

            _messageBus->SendMessage(msg);
        }
    }    
    else if(msg->_action == "Disarm")
    {
        begin_transaction();
        //update that person as a threat
        update_person(found_person,false,found_person.location());
        commit_transaction();
    }
}

void CampusDemo::Campus::MessageCallback(std::shared_ptr<message::Message> msg){
    
    bool gotSession = false;

    try
    {
        // what kind of message do we have?
        auto messageType = msg->get_message_type_name();

        if(messageType == message::message_types::action_message)
        {
            // begin a Gaia DB session
            begin_session();   
            gotSession = true;         
        
            auto actionMessage = reinterpret_cast<message::ActionMessage*>(msg.get());   

            //at this point we have our action message, update the DB
            if(actionMessage->_actorType == "Person")
                got_person_action_message(actionMessage);
        }
    }
    catch(const std::exception& e){
        log_this("CampusDemo::Campus::MessageCallback()", e);
    }
    catch(...){
        log_this("Exception in CampusDemo::Campus::MessageCallback() ...");
    }

    if(gotSession){
        // end the Gaia DB session
        end_session();
    }
}

void CampusDemo::Campus::StaticMessageCallback(std::shared_ptr<message::Message> msg){
    
    auto li = GetLastInstance();

    if(nullptr == li) //TODO: notify user
        return;

    li->MessageCallback(msg);        
}

int CampusDemo::Campus::RunAsync(){
    // start a new thread and ...

    return 0;
}

void CampusDemo::Campus::Worker(){
    //Initialize Gaia
    gaia::system::initialize(_config_file_name.c_str());     

    init_storage();

    //dump_db();

    while(true){
        usleep(_sleepTime);
    }
}

int CampusDemo::Campus::Init(std::shared_ptr<message::IMessageBus> messageBus){

    try{
        //Disregard all this for now
        //campus_ruleset_p_campus = std::shared_ptr<ICampus>(this);
        //auto bbb = std::shared_ptr<CampusDemo::Campus>(this);    
        //campus_ruleset_p_campus = std::shared_ptr<ICampus>(reinterpret_cast<ICampus*>(this))
        //campus_ruleset_p_campus = std::shared_ptr<ICampus>(reinterpret_cast<ICampus*>(this));
        //campus_ruleset_p_campus = reinterpret_cast<std::shared_ptr<ICampus>>(shared_from_this());

        //TODO : yes, I know, make this modern
        campus_ruleset_p_campus = reinterpret_cast<ICampus*>(this);

        if(nullptr == messageBus)
            throw std::invalid_argument("argument messageBus cannot be null");
        
        //Save the message bus and register a callback
        _messageBus = messageBus;
        _messageBus->RegisterMessageCallback(&CampusDemo::Campus::StaticMessageCallback, _sender_name);

        //Initialize Gaia
        gaia::system::initialize(_config_file_name.c_str());     

        init_storage();    
    }
    catch(const std::exception& e){
        log_this("CampusDemo::Campus::Init()", e);
        return -1;
    }
    catch(...){
        log_this("Exception in CampusDemo::Campus::Init() ...");
        return -1;
    }
    
    return 0;
}

//*** ICampus interface ***

void CampusDemo::Campus::cb_action( std::string actorType, 
    std::string actorName, std::string actionName, std::string arg1){

    if(nullptr == _messageBus)
        return;

    message::MessageHeader mh(_sequenceID++, _senderID, _senderName, _destID, _destName);

    std::shared_ptr<message::Message> msg = 
        std::make_shared<message::ActionMessage>(mh, actorType, actorName, actionName, arg1);

    _messageBus->SendMessage(msg);
}

void CampusDemo::Campus::cb_alert( std::string title, 
        std::string body, int severity, std::string arg1){

    if(nullptr == _messageBus)
        return;

    message::MessageHeader mh(_sequenceID++, _senderID, _senderName, _destID, _destName);

    std::shared_ptr<message::Message> msg = 
        std::make_shared<message::alert_message>(mh, title, body, severity, arg1);

    _messageBus->SendMessage(msg);
}

//*** DB Procedural ***

bool CampusDemo::Campus::get_person(const char* name, gaia::campus::person_t &found_person)
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

gaia_id_t CampusDemo::Campus::insert_campus(const char* name, bool in_emergency) {
    gaia::campus::campus_writer cw;
    cw.name = name;
    cw.in_emergency = in_emergency;
    UNUSED(in_emergency);
    return cw.insert_row();
}

void CampusDemo::Campus::update_campus(gaia::campus::campus_t& camp, bool in_emergency) {
    auto c = camp.writer();
    c.in_emergency = in_emergency;
    c.update_row();
}

gaia_id_t CampusDemo::Campus::insert_person(const char* name, bool is_threat, const std::string location) {
    gaia::campus::person_writer p;
    p.name = name;
    p.is_threat = is_threat;
    p.location = location;
    return p.insert_row();
}

void CampusDemo::Campus::update_person(gaia::campus::person_t& person, bool is_threat, const std::string location) {
    auto p = person.writer();
    p.is_threat = is_threat;
    p.location = location;
    p.update_row();
}

void CampusDemo::Campus::restore_default_values() {
    for (auto& person : gaia::campus::person_t::list()) {
        //_persons_v.push_back(person);
        update_person(person, false, "*");
    }
}

void CampusDemo::Campus::init_storage() {
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);

    if (gaia::campus::person_t::get_first()) {
        restore_default_values();
        tx.commit();
        return;
    }

    gaia::campus::campus_t campus = gaia::campus::campus_t::get(insert_campus("AAA", false));
    campus.person_list().insert(gaia::campus::person_t::insert_row("Unidentified", 0, "*"));
    campus.person_list().insert(gaia::campus::person_t::insert_row("Bob Kabob", 0, "*"));
    campus.person_list().insert(gaia::campus::person_t::insert_row("Sam Kabam", 0, "*"));

    tx.commit();
}