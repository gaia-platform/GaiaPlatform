/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "../inc/campus_demo.hpp"

CampusDemo::Campus::Campus(){}

CampusDemo::Campus::~Campus(){}

int CampusDemo::Campus::DemoTest(){
    return 0;
}

CampusDemo::Campus* CampusDemo::Campus::GetLastInstance(){
    return _lastInstance;
}

void CampusDemo::Campus::MessageCallback(std::shared_ptr<message::Message> msg){
    
    //TODO: ok for now, but we'll need to check type before casting
    auto actionMessage = reinterpret_cast<message::ActionMessage*>(msg.get());   

    //at this point we have our action message, update the DB

    UNUSED(actionMessage);
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

int CampusDemo::Campus::Init(std::shared_ptr<message::IMessageBus> messageBus){

    if(nullptr == messageBus)
    {
        //TODO: Throw probably
        return -1;
    }
    
    _messageBus = messageBus;
    _messageBus->RegisterMessageCallback(&CampusDemo::Campus::StaticMessageCallback);

    //Initialize Gaia
    gaia::system::initialize();       //TODO: translator not currently building

    return 0;
}
