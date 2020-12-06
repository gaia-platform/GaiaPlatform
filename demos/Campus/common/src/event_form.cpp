#include <form.h>
#include <string.h>

void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color);

int main()
{        
    FIELD *field[13];        
    FORM  *the_form;        
    WINDOW *the_form_win;
    int ch, rows, cols;        
    
    /* Initialize curses */        
    initscr();        
    start_color();        
    cbreak();        
    noecho();        
    keypad(stdscr, TRUE);        
    
    /* Initialize few color pairs */        
    
    init_pair(1, COLOR_RED, COLOR_BLACK);         

    // lables        
    field[0] = new_field(1, 10, 2, 1, 0, 0);        
    field[1] = new_field(1, 10, 3, 1, 0, 0);      
    field[2] = new_field(1, 10, 4, 1, 0, 0);      
    field[3] = new_field(1, 10, 5, 1, 0, 0);      
    field[4] = new_field(1, 10, 6, 1, 0, 0);      
    field[5] = new_field(1, 10, 7, 1, 0, 0);

    field_opts_off(field[0], O_ACTIVE);
    field_opts_off(field[1], O_ACTIVE);     
    field_opts_off(field[2], O_ACTIVE);
    field_opts_off(field[3], O_ACTIVE);     
    field_opts_off(field[4], O_ACTIVE);
    field_opts_off(field[5], O_ACTIVE); 

    set_field_buffer(field[0], 0, "Name:");
    set_field_buffer(field[1], 0, "Teacher:");
    set_field_buffer(field[2], 0, "Date:");
    set_field_buffer(field[3], 0, "Start:");
    set_field_buffer(field[4], 0, "End:");
    set_field_buffer(field[5], 0, "Room:");  

    // entry    
    
    field[6] = new_field(1, 10, 2, 10, 0, 0);        
    field[7] = new_field(1, 10, 3, 10, 0, 0);      
    field[8] = new_field(1, 10, 4, 10, 0, 0);        
    field[9] = new_field(1, 10, 5, 10, 0, 0);      
    field[10] = new_field(1, 10, 6, 10, 0, 0);        
    field[11] = new_field(1, 10, 7, 10, 0, 0);        
    field[12] = NULL;      

     
    /* Set field options */        
    set_field_back(field[6], A_UNDERLINE);        
    field_opts_off(field[6], O_AUTOSKIP); 
       
    set_field_back(field[7], A_UNDERLINE);         
    field_opts_off(field[7], O_AUTOSKIP);        
       
    set_field_back(field[8], A_UNDERLINE);         
    field_opts_off(field[8], O_AUTOSKIP);        
       
    set_field_back(field[9], A_UNDERLINE);         
    field_opts_off(field[9], O_AUTOSKIP);        
       
    set_field_back(field[10], A_UNDERLINE);         
    field_opts_off(field[10], O_AUTOSKIP);        
       
    set_field_back(field[11], A_UNDERLINE);         
    field_opts_off(field[11], O_AUTOSKIP);        
    
    /* Create the form and post it */        
    the_form = new_form(field);        
    
    /* Calculate the area required for the form */        
    scale_form(the_form, &rows, &cols);        
    
    /* Create the window to be associated with the form */        
    the_form_win = newwin(rows + 4, cols + 4, 4, 4);        
    keypad(the_form_win, TRUE);        
    
    /* Set main window and sub window */        
    set_form_win(the_form, the_form_win);        
    set_form_sub(the_form, derwin(the_form_win, rows, cols, 2, 2));        
    
    /* Print a border around the main window and print a title */        
    box(the_form_win, 0, 0);        
    print_in_middle(the_form_win, 1, 0, cols + 4, "Event", COLOR_PAIR(1));        
    post_form(the_form);        
    wrefresh(the_form_win);        
    mvprintw(LINES - 2, 0, "Use UP, DOWN arrow keys to switch between fields");        
    refresh();        
    
    /* Loop through to get user requests */        
    while((ch = wgetch(the_form_win)) != KEY_F(1))        
    {       
        switch(ch)                
        {       
            case KEY_DOWN:                                
            /* Go to next field */                                
            form_driver(the_form, REQ_NEXT_FIELD);                                
            /* Go to the end of the present buffer */                                
            /* Leaves nicely at the last character */                                
            form_driver(the_form, REQ_END_LINE);                                
            break;                        
            
            case KEY_UP:                                
            /* Go to previous field */                                
            form_driver(the_form, REQ_PREV_FIELD);                                
            form_driver(the_form, REQ_END_LINE);
            break;                        
            
            default:                                
            /* If this is a normal character, it gets */                                
            /* Printed                                */                                    
            form_driver(the_form, ch);                                
            break;                
        }    

        // read form data
        form_driver(the_form, REQ_VALIDATION);
        auto fb = field_buffer(field[0],0);  
        int gg = strlen(fb); 
    }        
    
    /* Un post form and free the memory */        
    unpost_form(the_form);        
    free_form(the_form);        
    free_field(field[0]);        
    free_field(field[1]);         
    endwin();        
    
    return 0;
}

void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color)
{       
    int length, x, y;        
    float temp;        

    if(win == NULL)                
        win = stdscr;        
        
    getyx(win, y, x);        
    
    if(startx != 0)                
        x = startx;        
        
    if(starty != 0)                
        y = starty;        
        
    if(width == 0)                
        width = 80;        
        
    length = strlen(string);        
    temp = (width - length)/ 2;        
    x = startx + (int)temp;        
    wattron(win, color);        
    mvwprintw(win, y, x, "%s", string);        
    wattroff(win, color);        
    refresh();
}