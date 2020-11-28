/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// This is the UI for the direct demo, it is messy right now, just ignore, it will be changed a lot

#include <menu.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <functional>
#include "../inc/campus_demo.hpp"

#if defined MESSAGE_BUS_MONOLITH
#include "../../monolith/inc/message_bus_monolith.hpp"
#elif defined MESSAGE_BUS_EDGE
#include "../../edge/inc/message_bus_edge.hpp"
#endif

// to supress unused-parameter build warnings
#define UNUSED(...) (void)(__VA_ARGS__)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0])) 
#define CTRLD 4

// These will be pulled from DB
char * choices1[] = {(char*)"Person", (char*)"Car", (char*)"Exit", (char*)NULL};
char * people[] = {(char*)"Unidentified", (char*)"Bob Kabob", (char*)"Sam Kabam", (char*)"Exit", (char*)NULL};
char * cars[] = {(char*)"Unidentified", (char*)"Ford Fairlane", (char*)"Purple Lambo", (char*)"Exit", (char*)NULL};

char * personAction[] = {(char*)"Move To", (char*)"Change Role", (char*)"Brandish Weapon", (char*)"Exit", (char*)NULL};
char * carAction[] = {(char*)"Move To", (char*)"Exit", (char*)NULL};

char * personLocations[] = {(char*)"Front Door", (char*)"Lab", (char*)"Main Hall", (char*)"Exit", (char*)NULL};
char * personRoles[] = {(char*)"Stranger", (char*)"Student", (char*)"Teacher", (char*)"Parent", (char*)"Exit", (char*)NULL};
char * carLocations[] = {(char*)"Entry", (char*)"Garage", (char*)"Concourse", (char*)"Exit", (char*)NULL};

class terminalMenu
{

private:

// should the UI show all mesages received?
bool _show_all_messages = true;

// the name of the client on the message bus
const std::string _sender_name = "termUi";

// singletonish
inline static terminalMenu* _lastInstance = nullptr;

// callback method type
typedef void (terminalMenu::*CallbackType)(char * name);  

// callback method container
struct CallbackContStruct
{
    CallbackType Callback;
};

// the message bus we want to use
std::shared_ptr<message::IMessageBus> _messageBus = nullptr;

// message header data
int _sequenceID = 0;
int _senderID = 0;
std::string _senderName = _sender_name;
int _destID = 0;
std::string _destName = "*";

// action data
std::string _actorType;
std::string _actorName;
std::string _actionName;
std::string _arg1;

/**
* ---
*
* @param[in] char *name
* @param[in] char* items[]
* @param[in] int starty
* @param[in] int startx
* @param[in] int width
* @param[in] char* string
* @param[in] chtype color
* @return void
* @throws 
* @exceptsafe yes
*/  
void putMenu(char *name, ITEM** items, int h, int w, int row, int col)
{
    int c;
    MENU* theMenu;
    WINDOW* menuWindow;
    bool exit = false;

    // Create menu 
    theMenu = new_menu((ITEM**)items); 
    
    // Create the window to be associated with the menu 
    menuWindow = newwin(h,w,row,col);
    keypad(menuWindow, TRUE); 
    
    // Set main window and sub window 
    set_menu_win(theMenu, menuWindow);
    set_menu_sub(theMenu, derwin(menuWindow, 6, 38, 3, 1));
    set_menu_format(theMenu, 5, 1); 
    
    // Set menu mark to the string " * " 
    set_menu_mark(theMenu, " * "); 
    
    // Print a border around the main window and print a title 
    box(menuWindow, 0, 0);
    print_in_middle(menuWindow, 1, 0, 40, name, COLOR_PAIR(1));
    mvwaddch(menuWindow, 2, 0, ACS_LTEE);
    mvwhline(menuWindow, 2, 1, ACS_HLINE, 38);
    mvwaddch(menuWindow, 2, 39, ACS_RTEE); 
    
    // Post the menu 
    post_menu(theMenu);
    wrefresh(menuWindow);
    attron(COLOR_PAIR(2));
    mvprintw(LINES - 2, 0, "Use PageUp and PageDown to scoll down or up a page of items");
    mvprintw(LINES - 1, 0, "Arrow Keys to navigate (F1 to Exit)");
    attroff(COLOR_PAIR(2));
    refresh();

    while ((c = wgetch(menuWindow)) != KEY_F(1))
    {
        switch (c)
        {
        case KEY_DOWN:
            menu_driver(theMenu, REQ_DOWN_ITEM);
            break;
        case KEY_UP:
            menu_driver(theMenu, REQ_UP_ITEM);
            break;
        case KEY_NPAGE:
            menu_driver(theMenu, REQ_SCR_DPAGE);
            break;
        case KEY_PPAGE:
            menu_driver(theMenu, REQ_SCR_UPAGE);
            break;

        case 10: // Enter                   
            {       
                ITEM *cur;                                                      
                cur = current_item(theMenu);        

                if(0 == strcmp(item_name(cur),"Exit"))     
                    exit = true;
                else
                {
                    CallbackContStruct* cbc = reinterpret_cast<CallbackContStruct*>(item_userptr(cur));    
                    std::invoke(cbc->Callback, this, (char *)item_name(cur));
                    pos_menu_cursor(theMenu);                     
                }
                                            
                break;                        
            }                        
            break;


        }
        wrefresh(menuWindow);

        if(exit)
            break;
    } 
    
    // Remove menu
    unpost_menu(theMenu);
    free_menu(theMenu);    

    // Remove window
    wborder(menuWindow, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '); // Erase frame around the window
    wrefresh(menuWindow); 
    delwin(menuWindow); 

    // Clear header
    move(row+1, col);
    clrtoeol();     
    move(row+2, col);
    clrtoeol(); 

    refresh();
}

/**
* ---
*
* @param[in] char *name
* @param[in] char* items[]
* @param[in] CallbackType handler
* @param[in] int starty
* @param[in] int startx
* @param[in] int width
* @param[in] char* string
* @param[in] chtype color
* @return void
* @throws 
* @exceptsafe yes
*/  
void putMenu(char *name, char* items[], CallbackType handler, int count, int h, int w, int row, int col)
{
    ITEM** menuItems;
    int n_choices, i;

    n_choices = count;
    menuItems = (ITEM**)calloc(n_choices, sizeof(ITEM*));

    CallbackContStruct cbc;
    cbc.Callback = handler;

    for (i = 0; i < n_choices; ++i)
    {
        menuItems[i] = new_item(items[i], ""); 
        set_item_userptr(menuItems[i], reinterpret_cast<void *>(&cbc));
    }

    putMenu((char *)name, menuItems, h, w, row, col);

    for (i = 0; i < n_choices; ++i)
        free_item(menuItems[i]);
}

/**
* ---
*
* @param[in] WINDOW* win
* @param[in] int starty
* @param[in] int startx
* @param[in] int width
* @param[in] char* string
* @param[in] chtype color
* @return void
* @throws 
* @exceptsafe yes
*/  
void print_in_middle(WINDOW* win, int starty, int startx, int width, char* string, chtype color)
{
    int length, x, y;
    float temp;

    if (win == NULL)
        win = stdscr;

    getyx(win, y, x);

    if (startx != 0)
        x = startx;

    if (starty != 0)
        y = starty;

    if (width == 0)
        width = 80;

    length = strlen(string);
    temp = (width - length) / 2;
    x = startx + (int)temp;
    wattron(win, color);
    mvwprintw(win, y, x, "%s", string);
    wattroff(win, color);
    refresh();
}

/**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/  
void showSelection(char * name)
{
    move(2, 0);        
    clrtoeol();        
    mvprintw(2, 0, "Item selected is : %s", name);
    refresh();
}

/**
* ---
*
* @param[in] char *textMessage
* @return void
* @throws 
* @exceptsafe yes
*/  
void showTextMessage(char * textMessage)
{
    move(2, 0);        
    clrtoeol();        
    mvprintw(2, 0, "%s", textMessage);
    refresh();
}

/**
* Display any kind of message on the UI
*
* @param[in] char *textMessage
* @return void
* @throws 
* @exceptsafe yes
*/  
void showMessage(std::shared_ptr<message::Message> msg)
{
    auto messageType = msg->get_message_type_name();
    char buffer[1024];

    if(messageType == message::message_types::action_message)
    {
        auto actionMessage = reinterpret_cast<message::ActionMessage*>(msg.get());   

        sprintf(buffer, "Change detected: %s %s %s", actionMessage->_actor.c_str(), 
            actionMessage->_action.c_str(), actionMessage->_arg1.c_str());
    }
    else if(messageType == message::message_types::alert_message)
    {
        auto alertMessage = reinterpret_cast<message::alert_message*>(msg.get());  

        std::string sev_level = "Unknown"; 

        switch(alertMessage->_severity)
        {
            case message::alert_message::severity_level_enum::alert :
                sev_level = "Alert";
            break;
            case message::alert_message::severity_level_enum::emergency :
                sev_level = "Emergency";
            break;
            case message::alert_message::severity_level_enum::notice :
                sev_level = "Notice";
            break;
        }

        sprintf(buffer, "Alert Level: %s, %s : %s %s", sev_level.c_str(),
            alertMessage->_title.c_str(), alertMessage->_body.c_str(), 
            alertMessage->_arg1.c_str());
    }

    showTextMessage(buffer);
}

/**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/  
void ActorTypeSelected(char *name)
{       
    // set action values
    _actorType = name;
    _actorName = "";
    _actionName = "";
    _arg1 = "";

    //showSelection(name);

    // show sub menu
    if(0 == strcmp(name, "Person"))
        putMenu((char *)"Actor", people, &terminalMenu::PersonSelected, ARRAY_SIZE(people), 10, 40, 4, 44);
    else if(0 == strcmp(name, "Car"))
        putMenu((char *)"Actor", cars, &terminalMenu::CarSelected, ARRAY_SIZE(cars), 10, 40, 4, 44);     
} 

/**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/  
void PersonSelected(char *name)
{        
    // set action values
    _actorName = name;
    _actionName = "";
    _arg1 = "";

    //showSelection(name);

    // show sub menu
    putMenu((char *)"Action", personAction, &terminalMenu::PersonsActionSelected, ARRAY_SIZE(personAction), 10, 40, 4, 84);   
} 

/**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/  
void CarSelected(char *name)
{        
    // set action values
    _actorName = name;
    _actionName = "";
    _arg1 = "";

    //showSelection(name);

    // show sub menu
    putMenu((char *)"Action", carAction, &terminalMenu::CarActionSelected, ARRAY_SIZE(carAction), 10, 40, 4, 84);   
} 

/**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/  
void PersonsActionSelected(char *name)
{        
    // set action values
    _actionName = name;
    _arg1 = "";

    //showSelection(name);

    // show sub menu
    if(0 == strcmp(name, "Move To"))
        putMenu((char *)"Location", personLocations, &terminalMenu::PersonsActionMovetoSelected, ARRAY_SIZE(personLocations), 10, 40, 4, 124);
    else if(0 == strcmp(name, "Change Role"))
        putMenu((char *)"New Role", personRoles, &terminalMenu::PersonsActionChangeRoleSelected, ARRAY_SIZE(personRoles), 10, 40, 4, 124);   
    else if(0 == strcmp(name, "Brandish Weapon"))
        DoTheChange();
} 

/**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/  
void CarActionSelected(char *name)
{        
    // set action values
    _actionName = name;
    _arg1 = "";

    putMenu((char *)"Location", carLocations, &terminalMenu::CarActionChangeLocationSelected, ARRAY_SIZE(carLocations), 10, 40, 4, 124);   
} 

/**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/  
void PersonsActionMovetoSelected(char *name)
{        
    // set action values
    _arg1 = name;

    //do the change  
    DoTheChange();
} 

/**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/  
void PersonsActionChangeRoleSelected(char *name)
{        
    // set action values
    _arg1 = name;

    //do the change  
    DoTheChange();  
} 

/**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/  
void CarActionChangeLocationSelected(char *name)
{        
    // set action values
    _arg1 = name;

    //do the change  
    DoTheChange();
} 

/**
* Generate a change message and send it to the message bus
*
* @return void
* @throws 
* @exceptsafe yes
*/  
void DoTheChange()
{
    if(nullptr == _messageBus)
        return;

    message::MessageHeader mh(_sequenceID++, _senderID, _senderName, _destID, _destName);

    std::shared_ptr<message::Message> msg = 
        std::make_shared<message::ActionMessage>(mh, _actorType, _actorName, _actionName, _arg1);

    _messageBus->SendMessage(msg);
}

public:

static terminalMenu* GetLastInstance()
{
    return _lastInstance;
}

std::shared_ptr<message::IMessageBus> GetMessageBus()
{
    return _messageBus;
}

/**
* Blocking Run
*
* @param[in] 
* @param[out] 
* @return 
* @throws 
* @exceptsafe yes
*/  
void Run()
{    
    putMenu((char *)"Actor Type", choices1, &terminalMenu::ActorTypeSelected, ARRAY_SIZE(choices1), 10, 40, 4, 4); 
    endwin();
}

/**
* Callback from the message bus when a message arrives
*
* @param[in] message::Message msg
* @return 
* @throws 
* @exceptsafe yes
*/  
void MessageCallback(std::shared_ptr<message::Message> msg)
{
    if(_show_all_messages)
        showMessage(msg);

    auto messageType = msg->get_message_type_name();

    // switch on the type of message received
    if(messageType == message::message_types::action_message)
    {
        //auto actionMessage = reinterpret_cast<message::ActionMessage*>(msg.get());   

        //at this point we have our action message, update the DB
        //if(actionMessage->_actorType == "Person")
            //got_person_action_message(actionMessage);
    }
}

/**
* Callback from the message bus when a message arrives
* Am not crazy about this, I need to convert to using a std::fuction
*
* @param[in] message::Message msg
* @return 
* @throws 
* @exceptsafe yes
*/  
static void StaticMessageCallback(std::shared_ptr<message::Message> msg)
{
    auto li = GetLastInstance();

    if(nullptr == li) //TODO: notify user
        return;

    li->MessageCallback(msg);
}

/**
* Initialize curses
*
* @param[in] 
* @param[out] 
* @return 
* @throws 
* @exceptsafe yes
*/  
void Init()
{   
    _lastInstance = this;

    // Initialize ncurses
    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_CYAN, COLOR_BLACK); 

    // function of member
    //std::function<void(terminalMenu&, std::shared_ptr<message::Message> msg)> mcb = &terminalMenu::MessageCallback;

    // Initialize message bus
    _messageBus = std::make_shared<message::MessageBus>();
    _messageBus->RegisterMessageCallback(&terminalMenu::StaticMessageCallback, _sender_name);
}

};

/**
* ---
*
* @param[in] 
* @param[out] 
* @return 
* @throws 
* @exceptsafe yes
*/  
int main()
{
    terminalMenu tm;
    tm.Init();

    CampusDemo::Campus cd;

    if( 0 == cd.Init(tm.GetMessageBus()))
        tm.Run();
    else
    {
        std::cout << "nope";
    }
    
}