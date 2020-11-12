#pragma once
/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include "message.hpp" 
#include "message_bus.hpp"

#include "gaia_system.hpp"

//using namespace gaia::common;
//using namespace gaia::db;
//using namespace gaia::rules;


using namespace std;

namespace CampusDemo{
class Campus{

private:

    // singletonish
    inline static Campus* _lastInstance = nullptr;

    // the message bus we want to use
    std::shared_ptr<message::IMessageBus> _messageBus = nullptr;

    /**
     * Run the campus demo.
     *
     * @param[in] std::shared_ptr<message::IMessageBus> messageBus
     * @return 
     * @throws
     * @exceptsafe basic?
     */
    int Init(std::shared_ptr<message::IMessageBus> messageBus);

    static Campus* GetLastInstance();

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
;

public:

    Campus();

    ~Campus();

    int DemoTest();

    /**
     * Run the campus demo.
     *
     * @param[out,in] 
     * @return 
     * @throws
     * @exceptsafe basic?
     */
    int RunAsync();

}; // class Campus
} // namespace CampusDemo
