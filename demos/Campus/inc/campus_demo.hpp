#pragma once
/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include "message.hpp" 
#include "message_bus.hpp"

/*
#include "gaia_system.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::rules;
*/

using namespace std;

namespace CampusDemo{
class Campus{

private:

    // the message bus we want to use
    std::shared_ptr<message::IMessageBus> _messageBus = nullptr;

    /**
     * Run the campus demo.
     *
     * @param[out,in] 
     * @return 
     * @throws
     * @exceptsafe basic?
     */
    int Init();

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
    int Run();

}; // class Campus
} // namespace CampusDemo
