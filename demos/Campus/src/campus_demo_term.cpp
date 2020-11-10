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

// callback method type
typedef void (terminalMenu::*CallbackType)(char * name);  

// callback method container
struct CallbackContStruct
{
    CallbackType Callback;
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

    refresh();
}


/**
* ---
*
* @param[in] 
* @param[out] 
* @return 
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
* @param[in] 
* @param[out] 
* @return 
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
* @param[in] 
* @param[out] 
* @return 
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
* @param[in] 
* @param[out] 
* @return 
* @throws 
* @exceptsafe yes
*/  
void ActorTypeSelected(char *name)
{       
    showSelection(name);

    if(0 == strcmp(name, "Person"))
        putMenu((char *)"Actor", people, &terminalMenu::PersonSelected, ARRAY_SIZE(people), 10, 40, 4, 44);
    else if(0 == strcmp(name, "Car"))
        putMenu((char *)"Actor", cars, &terminalMenu::CarSelected, ARRAY_SIZE(cars), 10, 40, 4, 44);     
} 

/**
* ---
*
* @param[in] 
* @param[out] 
* @return 
* @throws 
* @exceptsafe yes
*/  
void PersonSelected(char *name)
{        
    showSelection(name);
    putMenu((char *)"Action", personAction, &terminalMenu::PersonsActionSelected, ARRAY_SIZE(personAction), 10, 40, 4, 84);   
} 

/**
* ---
*
* @param[in] 
* @param[out] 
* @return 
* @throws 
* @exceptsafe yes
*/  
void CarSelected(char *name)
{        
    showSelection(name);
    putMenu((char *)"Action", carAction, &terminalMenu::CarActionSelected, ARRAY_SIZE(carAction), 10, 40, 4, 84);   
} 

/**
* ---
*
* @param[in] 
* @param[out] 
* @return 
* @throws 
* @exceptsafe yes
*/  
void PersonsActionSelected(char *name)
{        
    showSelection(name);

    if(0 == strcmp(name, "Move To"))
        putMenu((char *)"Location", personLocations, &terminalMenu::PersonsActionMovetoSelected, ARRAY_SIZE(personLocations), 10, 40, 4, 124);
    else if(0 == strcmp(name, "Change Role"))
        putMenu((char *)"New Role", personRoles, &terminalMenu::PersonsActionChangeRoleSelected, ARRAY_SIZE(personRoles), 10, 40, 4, 124);    
} 

/**
* ---
*
* @param[in] 
* @param[out] 
* @return 
* @throws 
* @exceptsafe yes
*/  
void CarActionSelected(char *name)
{        
    showSelection(name);
    putMenu((char *)"Location", carLocations, &terminalMenu::CarActionChangeLocationSelected, ARRAY_SIZE(carLocations), 10, 40, 4, 124);   
} 

/**
* ---
*
* @param[in] 
* @param[out] 
* @return 
* @throws 
* @exceptsafe yes
*/  
void PersonsActionMovetoSelected(char *name)
{        
    showSelection(name);
    //do the change  
} 

/**
* ---
*
* @param[in] 
* @param[out] 
* @return 
* @throws 
* @exceptsafe yes
*/  
void PersonsActionChangeRoleSelected(char *name)
{        
    showSelection(name);
    //do the change    
} 

/**
* ---
*
* @param[in] 
* @param[out] 
* @return 
* @throws 
* @exceptsafe yes
*/  
void CarActionChangeLocationSelected(char *name)
{        
    showSelection(name);
    //do the change    
} 

public:

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
    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_CYAN, COLOR_BLACK); 
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
    terminalMenu tw;
    tw.Init();
    tw.Run();
}