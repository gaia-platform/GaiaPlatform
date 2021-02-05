/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// This is the UI for the direct demo, it is messy right now, just ignore, it will be changed a lot

#include <menu.h>
#include <ncurses.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <functional>

#include "../inc/campus_demo.hpp"
#include "../inc/nv_form.hpp"

#if defined MESSAGE_BUS_MONOLITH
#include "../../monolith/inc/message_bus_monolith.hpp"
#elif defined MESSAGE_BUS_EDGE
#include "../../edge/inc/message_bus_edge.hpp"
#endif

class terminal_menu
{

private:
    // starting row (top) of menu
    const int m_menu_row = 8;

    // message window
    WINDOW* m_message_window;

    // should the UI show all mesages received?
    bool m_show_all_messages = true;

    // the name of the client on the message bus
    const std::string m_sender_name_c = "termUi";

    // singleton
    inline static terminal_menu* m_last_instance = nullptr;

    // callback method type
    typedef void (terminal_menu::*callback_type)(char* name);

    // callback method container
    struct callback_cont_t
    {
        callback_type callback;
    };

    // the message bus we want to use
    std::shared_ptr<message::i_message_bus> m_messageBus = nullptr;

    // message header data
    int m_sequenceID = 0;
    int m_senderID = 0;
    std::string m_sender_name = m_sender_name_c;
    int m_destID = 0;
    std::string m_dest_name = "*";

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
    void put_menu(char* name, ITEM** items, int h, int w, int row, int col)
    {
        int c;
        MENU* the_menu;
        WINDOW* menu_mindow;
        bool exit = false;

        const int lines = 6;
        const int cols = 38;
        const int ten = 10;
        const int five = 5;
        const int middle_width = 40;

        // Create menu
        the_menu = new_menu(items);

        // Create the window to be associated with the menu
        menu_mindow = newwin(h, w, row, col);
        keypad(menu_mindow, TRUE);

        // Set main window and sub window
        set_menu_win(the_menu, menu_mindow);
        set_menu_sub(the_menu, derwin(menu_mindow, lines, cols, 3, 1));
        set_menu_format(the_menu, five, 1);

        // Set menu mark to the string " * "
        set_menu_mark(the_menu, " * ");

        // Print a border around the main window and print a title
        box(menu_mindow, 0, 0);
        print_in_middle(menu_mindow, 1, 0, middle_width, name, COLOR_PAIR(1));
        mvwaddch(menu_mindow, 2, 0, ACS_LTEE);
        mvwhline(menu_mindow, 2, 1, ACS_HLINE, 38);
        mvwaddch(menu_mindow, 2, 39, ACS_RTEE);

        // Post the menu
        post_menu(the_menu);
        wrefresh(menu_mindow);
        attron(COLOR_PAIR(2));
        mvprintw(LINES - 2, 0, "Use PageUp and PageDown to scoll down or up a page of items");
        mvprintw(LINES - 1, 0, "Arrow Keys to navigate");
        attroff(COLOR_PAIR(2));
        refresh();

        while ((c = wgetch(menu_mindow)) != KEY_F(1))
        {
            switch (c)
            {
            case KEY_DOWN:
                menu_driver(the_menu, REQ_DOWN_ITEM);
                break;
            case KEY_UP:
                menu_driver(the_menu, REQ_UP_ITEM);
                break;
            case KEY_NPAGE:
                menu_driver(the_menu, REQ_SCR_DPAGE);
                break;
            case KEY_PPAGE:
                menu_driver(the_menu, REQ_SCR_UPAGE);
                break;

            case ten: // Enter
            {
                ITEM* cur;
                cur = current_item(the_menu);

                if (0 == strcmp(item_name(cur), "Exit"))
                    exit = true;
                else
                {
                    auto cbc = reinterpret_cast<callback_cont_t*>(item_userptr(cur));
                    std::invoke(cbc->callback, this, (char*)item_name(cur));
                    pos_menu_cursor(the_menu);
                }

                break;
            }
            break;
            }
            wrefresh(menu_mindow);

            if (exit)
                break;
        }

        // Remove menu
        unpost_menu(the_menu);
        free_menu(the_menu);

        // Remove window
        wborder(menu_mindow, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '); // Erase frame around the window
        wrefresh(menu_mindow);
        delwin(menu_mindow);

        // Clear header
        move(row + 1, col);
        clrtoeol();
        move(row + 2, col);
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
    void put_menu(char* name, char* items[], callback_type handler, int count, int h, int w, int row, int col)
    {
        ITEM** menu_items;
        int n_choices, i;

        n_choices = count;
        menu_items = static_cast<ITEM**>(calloc(n_choices, sizeof(ITEM*)));

        callback_cont_t cbc;
        cbc.callback = handler;

        for (i = 0; i < n_choices; ++i)
        {
            menu_items[i] = new_item(items[i], "");
            set_item_userptr(menu_items[i], reinterpret_cast<void*>(&cbc));
        }

        put_menu(name, menu_items, h, w, row, col);

        for (i = 0; i < n_choices; ++i)
            free_item(menu_items[i]);
    }

    /**
* ---
*
* @param[in] char *name
* @param[in] std::vector<std::string> items
* @param[in] CallbackType handler
* @param[in] int h
* @param[in] int w
* @param[in] int row
* @param[in] int col
* @return void
* @throws 
* @exceptsafe yes
*/
    void put_menu(char* name, std::vector<std::string> items, callback_type handler, int h, int w, int row, int col)
    {
        ITEM** menu_items;
        int n_choices, i;

        n_choices = items.size();
        menu_items = static_cast<ITEM**>(calloc(n_choices + 2, sizeof(ITEM*)));

        callback_cont_t cbc;
        cbc.callback = handler;

        for (i = 0; i < n_choices; ++i)
        {
            menu_items[i] = new_item(items[i].c_str(), "");
            set_item_userptr(menu_items[i], reinterpret_cast<void*>(&cbc));
        }

        menu_items[i] = new_item("Exit", "");
        set_item_userptr(menu_items[i], reinterpret_cast<void*>(&cbc));

        menu_items[i + 1] = new_item(static_cast<char*>(nullptr), "");
        set_item_userptr(menu_items[i + 1], reinterpret_cast<void*>(&cbc));

        put_menu(name, menu_items, h, w, row, col);

        for (i = 0; i < n_choices + 2; ++i)
            free_item(menu_items[i]);
    }

const int eighty = 80;

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

        if (win == nullptr)
            win = stdscr;

        getyx(win, y, x);

        if (startx != 0)
            x = startx;

        if (starty != 0)
            y = starty;

        if (width == 0)
            width = eighty;

        length = strlen(string);
        temp = static_cast<float>(width - length) / 2;
        x = startx + static_cast<int>(temp);
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
    void show_selection(char* name)
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
        scrollok(m_message_window, TRUE);
    }

    /**
* Show a text message
*
* @param[in] char *textMessage
* @return void
* @throws 
* @exceptsafe yes
*/
    void show_text_message(char* text_message)
    {
        //move(2, 0);
        //clrtoeol();
        //mvprintw(2, 0, "%s", textMessage);
        //refresh();

        wprintw(m_message_window, "%s\n", text_message);
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
        const int buffsize = 1024;
        auto message_type = msg->get_message_type_name();
        char buffer[buffsize];

        if (message_type == bus_messages::message_types::action_message)
        {
            auto action_message = reinterpret_cast<bus_messages::action_message*>(msg.get());

            sprintf(buffer, "Change detected: %s %s %s", action_message->m_actor.c_str(), action_message->m_action.c_str(), action_message->m_arg1.c_str());
        }
        else if (message_type == bus_messages::message_types::alert_message)
        {
            auto alert_message = reinterpret_cast<bus_messages::alert_message*>(msg.get());

            std::string sev_level = "Unknown";

            switch (alert_message->m_severity)
            {
            case bus_messages::alert_message::severity_level_enum::alert:
                sev_level = "Alert";
                break;
            case bus_messages::alert_message::severity_level_enum::emergency:
                sev_level = "Emergency";
                break;
            case bus_messages::alert_message::severity_level_enum::notice:
                sev_level = "Notice";
                break;
            }

            sprintf(buffer, "Alert Level: %s, %s : %s %s", sev_level.c_str(), alert_message->m_title.c_str(), alert_message->m_body.c_str(), alert_message->m_arg1.c_str());
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
    void actor_type_selected(char* name)
    {
        // set action values
        m_actorType = name;
        m_actorName = "";
        m_actionName = "";
        m_arg1 = "";

        //showSelection(name);

        // show sub menu
        if (0 == strcmp(name, "Person"))
            put_menu((char*)"Actor", m_cdp->get_person_name_list(), &terminal_menu::person_selected, 10, 40, m_menu_row, 44);
        else if (0 == strcmp(name, "Car"))
            put_menu((char*)"Actor", m_cdp->get_car_list(), &terminal_menu::car_selected, 10, 40, m_menu_row, 44);
    }

    /**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/
    void person_selected(char* name)
    {
        // set action values
        m_actorName = name;
        m_actionName = "";
        m_arg1 = "";

        //showSelection(name);

        // show sub menu
        put_menu((char*)"Action", m_cdp->get_person_action_list(), &terminal_menu::persons_action_selected, 10, 40, m_menu_row, 84);
    }

    /**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/
    void car_selected(char* name)
    {
        // set action values
        m_actorName = name;
        m_actionName = "";
        m_arg1 = "";

        //showSelection(name);

        // show sub menu
        put_menu((char*)"Action", m_cdp->get_car_action_list(), &terminal_menu::car_action_selected, 10, 40, m_menu_row, 84);
    }

    /**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/
    void persons_action_selected(char* name)
    {
        // set action values
        m_actionName = name;
        m_arg1 = "";

        //showSelection(name);

        // show sub menu
        if (0 == strcmp(name, "Move To"))
            put_menu((char*)"Location", m_cdp->get_person_location_list(), &terminal_menu::persons_action_moveto_selected, 10, 40, m_menu_row, 124);
        else if (0 == strcmp(name, "Register For Event"))
            put_menu((char*)"Register For Event", m_cdp->get_event_name_list(), &terminal_menu::persons_action_register_for_event_selected, 10, 40, m_menu_row, 124);
        else if (0 == strcmp(name, "Change Role"))
            put_menu((char*)"New Role", m_cdp->get_person_role_list(), &terminal_menu::persons_action_change_role_selected, 10, 40, m_menu_row, 124);
        else if (0 == strcmp(name, "Brandish Weapon"))
            do_the_change();
        else if (0 == strcmp(name, "Disarm"))
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
    void car_action_selected(char* name)
    {
        // set action values
        m_actionName = name;
        m_arg1 = "";

        put_menu((char*)"Location", m_cdp->get_car_location_list(), &terminal_menu::car_action_change_location_selected, 10, 40, m_menu_row, 124);
    }

    /**
* ---
*
* @param[in] char *name
* @return void
* @throws 
* @exceptsafe yes
*/
    void persons_action_moveto_selected(char* name)
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
    void persons_action_register_for_event_selected(char* name)
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
    void persons_action_change_role_selected(char* name)
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
    void car_action_change_location_selected(char* name)
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
        if (nullptr == m_messageBus)
            return;

        bus_messages::message_header mh(m_sequenceID++, m_senderID, m_sender_name, m_destID, m_dest_name);

        std::shared_ptr<bus_messages::message> msg = std::make_shared<bus_messages::action_message>(mh, m_actorType, m_actorName, m_actionName, m_arg1);

        m_messageBus->send_message(msg);
    }

public:
    static terminal_menu* get_last_instance()
    {
        return m_last_instance;
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
        put_menu((char*)"Actor Type", m_cdp->get_actor_type_list(), &terminal_menu::actor_type_selected, 10, 40, m_menu_row, 4);

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
        if (m_show_all_messages)
            show_message(msg);

        auto message_type = msg->get_message_type_name();

        // switch on the type of message received
        if (message_type == bus_messages::message_types::action_message)
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

        if (nullptr == li) //TODO: notify user
            return;

        li->message_callback(msg);
    }

    std::shared_ptr<campus_demo::campus> m_cdp;

    /**
* Initialize curses
*
* @param[in] 
* @param[out] 
* @return 
* @throws 
* @exceptsafe yes
*/
    void init(std::shared_ptr<campus_demo::campus> cdp)
    {
        const int six = 6;
        m_last_instance = this;
        m_cdp = cdp;

        // Initialize ncurses
        initscr();
        start_color();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_CYAN, COLOR_BLACK);

        create_message_window(1, 1, six, 0);

        // function of member
        //std::function<void(terminalMenu&, std::shared_ptr<bus_messages::message> msg)> mcb = &terminalMenu::MessageCallback;

        try
        {
            // Initialize message bus
            m_messageBus = std::make_shared<message::message_bus>();
            m_messageBus->register_message_callback(&terminal_menu::static_message_callback, m_sender_name);
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        catch (...)
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

    try
    {
        //campus_demo::campus cd;
        terminal_menu tm;

        std::shared_ptr<campus_demo::campus> cdp = make_shared<campus_demo::campus>();

        tm.init(cdp);

        if (0 == cdp->init(tm.get_message_bus()))
            tm.run();
        else
        {
            std::cout << "nope";
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "Exception ..." << '\n';
    }

    return 0;
}
