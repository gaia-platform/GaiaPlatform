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
 #include "../inc/nv_form.hpp"

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
char * m_choices1[] = {(char*)"Person", (char*)"Car", (char*)"Exit", (char*)NULL};
char * m_people[] = {(char*)"Unidentified", (char*)"Bob Kabob", (char*)"Sam Kabam", (char*)"Exit", (char*)NULL};
char * m_cars[] = {(char*)"Unidentified", (char*)"Ford Fairlane", (char*)"Purple Lambo", (char*)"Exit", (char*)NULL};

char * m_personAction[] = {(char*)"Move To", (char*)"Change Role", (char*)"Brandish Weapon", (char*)"Disarm", (char*)"Exit", (char*)NULL};
char * m_carAction[] = {(char*)"Move To", (char*)"Exit", (char*)NULL};

char * m_personLocations[] = {(char*)"Front Door", (char*)"Lab", (char*)"Main Hall", (char*)"Exit", (char*)NULL};
char * m_personRoles[] = {(char*)"Stranger", (char*)"Student", (char*)"Teacher", (char*)"Parent", (char*)"Exit", (char*)NULL};
char * m_carLocations[] = {(char*)"Entry", (char*)"Garage", (char*)"Concourse", (char*)"Exit", (char*)NULL};

class terminal_menu
{

private:

// starting row (top) of menu
int m_menu_row = 8;

// message window
WINDOW *m_message_window;

// should the UI show all mesages received?
bool m_show_all_messages = true;

// the name of the client on the message bus
const std::string m_sender_name = "termUi";

// singleton
inline static terminal_menu* m_lastInstance = nullptr;

// callback method type
typedef void (terminal_menu::*callback_type)(char * name);  

// callback method container
struct callback_cont_struct
{
    callback_type Callback;
};

// the message bus we want to use
std::shared_ptr<message::i_message_bus> m_messageBus = nullptr;

// message header data
int m_sequenceID = 0;
int m_senderID = 0;
std::string m_senderName = m_sender_name;
int m_destID = 0;
std::string m_destName = "*";

// action data
std::string m_actorType;
std::string m_actorName;
std::string m_actionName;
std::string m_arg1;

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
void put_menu(char *name, ITEM** items, int h, int w, int row, int col)
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
                    callback_cont_struct* cbc = reinterpret_cast<callback_cont_struct*>(item_userptr(cur));    
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
void put_menu(char *name, char* items[], callback_type handler, int count, int h, int w, int row, int col)
{
    ITEM** menuItems;
    int n_choices, i;

    n_choices = count;
    menuItems = (ITEM**)calloc(n_choices, sizeof(ITEM*));

    callback_cont_struct cbc;
    cbc.Callback = handler;

    for (i = 0; i < n_choices; ++i)
    {
        menuItems[i] = new_item(items[i], ""); 
        set_item_userptr(menuItems[i], reinterpret_cast<void *>(&cbc));
    }

    put_menu((char *)name, menuItems, h, w, row, col);

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
* Show a selected item message
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/  
void show_selection(char * name)
{
    move(2, 0);        
    clrtoeol();        
    mvprintw(2, 0, "Item selected is : %s", name);
    refresh();
}

/**
* Create a scrolling text window to hold messages
*
* @param[in] int row
* @param[in] int col
* @param[in] int rows
* @param[in] int cols
* @return void
* @throws 
* @exceptsafe yes
*/  
void create_message_window(int row, int col, int rows, int cols)
{
    //int i = 2, height, width;
    //getmaxyx(stdscr, height, width);

    m_message_window = newwin(rows, cols, row, col);
    scrollok(m_message_window,TRUE);
}

/**
* Show a text message
*
* @param[in] char *textMessage
* @return void
* @throws 
* @exceptsafe yes
*/  
void show_text_message(char * textMessage)
{
    //move(2, 0);        
    //clrtoeol();        
    //mvprintw(2, 0, "%s", textMessage);
    //refresh();

    wprintw(m_message_window, "%s\n", textMessage);
    wrefresh(m_message_window);
}

/**
* Display any kind of message on the UI
*
* @param[in] char *textMessage
* @return void
* @throws 
* @exceptsafe yes
*/  
void show_message(std::shared_ptr<bus_messages::message> msg)
{
    auto messageType = msg->get_message_type_name();
    char buffer[1024];

    if(messageType == bus_messages::message_types::action_message)
    {
        auto action_message = reinterpret_cast<bus_messages::action_message*>(msg.get());   

        sprintf(buffer, "Change detected: %s %s %s", action_message->m_actor.c_str(), 
            action_message->m_action.c_str(), action_message->m_arg1.c_str());
    }
    else if(messageType == bus_messages::message_types::alert_message)
    {
        auto alertMessage = reinterpret_cast<bus_messages::alert_message*>(msg.get());  

        std::string sev_level = "Unknown"; 

        switch(alertMessage->m_severity)
        {
            case bus_messages::alert_message::severity_level_enum::alert :
                sev_level = "Alert";
            break;
            case bus_messages::alert_message::severity_level_enum::emergency :
                sev_level = "Emergency";
            break;
            case bus_messages::alert_message::severity_level_enum::notice :
                sev_level = "Notice";
            break;
        }

        sprintf(buffer, "Alert Level: %s, %s : %s %s", sev_level.c_str(),
            alertMessage->m_title.c_str(), alertMessage->m_body.c_str(), 
            alertMessage->m_arg1.c_str());
    }

    show_text_message(buffer);
}

/**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/  
void actor_type_selected(char *name)
{       
    // set action values
    m_actorType = name;
    m_actorName = "";
    m_actionName = "";
    m_arg1 = "";

    //showSelection(name);

    // show sub menu
    if(0 == strcmp(name, "Person"))
        put_menu((char *)"Actor", m_people, &terminal_menu::person_selected, ARRAY_SIZE(m_people), 10, 40, m_menu_row, 44);
    else if(0 == strcmp(name, "Car"))
        put_menu((char *)"Actor", m_cars, &terminal_menu::car_selected, ARRAY_SIZE(m_cars), 10, 40, m_menu_row, 44);     
} 

/**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/  
void person_selected(char *name)
{        
    // set action values
    m_actorName = name;
    m_actionName = "";
    m_arg1 = "";

    //showSelection(name);

    // show sub menu
    put_menu((char *)"Action", m_personAction, &terminal_menu::persons_action_selected, ARRAY_SIZE(m_personAction), 10, 40, m_menu_row, 84);   
} 

/**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/  
void car_selected(char *name)
{        
    // set action values
    m_actorName = name;
    m_actionName = "";
    m_arg1 = "";

    //showSelection(name);

    // show sub menu
    put_menu((char *)"Action", m_carAction, &terminal_menu::car_action_selected, ARRAY_SIZE(m_carAction), 10, 40, m_menu_row, 84);   
} 

/**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/  
void persons_action_selected(char *name)
{        
    // set action values
    m_actionName = name;
    m_arg1 = "";

    //showSelection(name);

    // show sub menu
    if(0 == strcmp(name, "Move To"))
        put_menu((char *)"Location", m_personLocations, &terminal_menu::persons_action_moveto_selected, ARRAY_SIZE(m_personLocations), 10, 40, m_menu_row, 124);
    else if(0 == strcmp(name, "Change Role"))
        put_menu((char *)"New Role", m_personRoles, &terminal_menu::persons_action_change_role_selected, ARRAY_SIZE(m_personRoles), 10, 40, m_menu_row, 124);   
    else if(0 == strcmp(name, "Brandish Weapon"))
        do_the_change();    
    else if(0 == strcmp(name, "Disarm"))
        do_the_change();
} 

/**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/  
void car_action_selected(char *name)
{        
    // set action values
    m_actionName = name;
    m_arg1 = "";

    put_menu((char *)"Location", m_carLocations, &terminal_menu::car_action_change_location_selected, ARRAY_SIZE(m_carLocations), 10, 40, m_menu_row, 124);   
} 

/**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/  
void persons_action_moveto_selected(char *name)
{        
    // set action values
    m_arg1 = name;

    //do the change  
    do_the_change();
} 

/**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/  
void persons_action_change_role_selected(char *name)
{        
    // set action values
    m_arg1 = name;

    //do the change  
    do_the_change();  
} 

/**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/  
void car_action_change_location_selected(char *name)
{        
    // set action values
    m_arg1 = name;

    //do the change  
    do_the_change();
} 

/**
* Generate a change message and send it to the message bus
*
* @return void
* @throws 
* @exceptsafe yes
*/  
void do_the_change()
{
    if(nullptr == m_messageBus)
        return;

    bus_messages::message_header mh(m_sequenceID++, m_senderID, m_senderName, m_destID, m_destName);

    std::shared_ptr<bus_messages::message> msg = 
        std::make_shared<bus_messages::action_message>(mh, m_actorType, m_actorName, m_actionName, m_arg1);

    m_messageBus->send_message(msg);
}

public:

static terminal_menu* get_last_instance()
{
    return m_lastInstance;
}

std::shared_ptr<message::i_message_bus> get_message_bus()
{
    return m_messageBus;
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
void run()
{    
    // show first menu
    put_menu((char *)"Actor Type", m_choices1, &terminal_menu::actor_type_selected, ARRAY_SIZE(m_choices1), 10, 40, m_menu_row, 4); 

    // shut down ncurses
    endwin();

    // shut down Gaia
    gaia::system::shutdown();
}

/**
* Callback from the message bus when a message arrives
*
* @param[in] bus_messages::message msg
* @return 
* @throws 
* @exceptsafe yes
*/  
void message_callback(std::shared_ptr<bus_messages::message> msg)
{
    if(m_show_all_messages)
        show_message(msg);

    auto messageType = msg->get_message_type_name();

    // switch on the type of message received
    if(messageType == bus_messages::message_types::action_message)
    {
        //auto action_message = reinterpret_cast<bus_messages::action_message*>(msg.get());   

        //at this point we have our action message, update the DB
        //if(action_message->_actorType == "Person")
            //got_person_action_message(action_message);
    }
}

/**
* Callback from the message bus when a message arrives
* Am not crazy about this, I need to convert to using a std::fuction
*
* @param[in] bus_messages::message msg
* @return 
* @throws 
* @exceptsafe yes
*/  
static void static_message_callback(std::shared_ptr<bus_messages::message> msg)
{
    auto li = get_last_instance();

    if(nullptr == li) //TODO: notify user
        return;

    li->message_callback(msg);
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
void init()
{   
    m_lastInstance = this;

    // Initialize ncurses
    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_CYAN, COLOR_BLACK); 

    create_message_window(1, 1, 6, 0);

    // function of member
    //std::function<void(terminalMenu&, std::shared_ptr<bus_messages::message> msg)> mcb = &terminalMenu::MessageCallback;

    try
    {
        // Initialize message bus
        m_messageBus = std::make_shared<message::message_bus>();
        m_messageBus->register_message_callback(&terminal_menu::static_message_callback, m_sender_name);        
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
    catch(...)
    {
        std::cerr << "Exception ..." << '\n';
    }
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
    /*nv_form ef;

    std::vector<form_nv_field> fields = { 
        form_nv_field("name", "Name:", ""),          
        form_nv_field("teacher", "Teacher:", ""),          
        form_nv_field("date", "Date:", ""),          
        form_nv_field("startTime", "Start:", ""),          
        form_nv_field("endTime", "End:", ""),         
        form_nv_field("room", "Room:", "") 
    };

    ef.init("Event", fields);
    ef.run();

    auto feelds = ef.get_fields();

        for(auto feeld : feelds){
        auto the_val = feeld.get_value().c_str();
        std::cout << the_val;
    }*/

    terminal_menu tm;
    tm.init();

    campus_demo::campus cd;

    if( 0 == cd.init(tm.get_message_bus()))
        tm.run();
    else
    {
        std::cout << "nope";
    }
    
    return 0;
}

