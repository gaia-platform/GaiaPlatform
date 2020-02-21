/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "event_types.h"

enum class event_mode {
    immediate, /**< execute any rules associated with this event is logged */ 
    deferred   /**< defer execution of any rules associated with this event until pending events are processed */
};

class event_manager
{
public:
    event_manager();

    /**
     * Sets up the 1:M mapping between events and rules.
     * Sets up 
     * information as well as event logging information
     */
    bool init();

    /**
     * Writes an event to the event log
     * 
     * @param event type of the event being logged
     * @param mode event_mode for deferred or immediate rule execution
     */
    bool log_event(on_row_change event, event_mode mode);

    /**
     * Writes an event to the event log
     * 
     * @param event type of the event being logged
     * @param mode event_mode for deferred or immediate rule execution
     */    
    bool log_event(on_col_change event, event_mode mode);


    /**
     * Commit events are always run as immediate so there
     * is no need for a mode parameter
     * 
     * @param event commit event type
     */
    bool log_event(on_tx_commit event);



private:
};