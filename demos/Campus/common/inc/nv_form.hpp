#include <form.h>
#include <string.h>

#include <iostream>
#include <vector>

class form_nv_field
{
private:
    std::string m_name;
    std::string m_label;
    std::string m_value;

public:
    std::string get_name()
    {
        return m_name;
    }

    std::string get_label()
    {
        return m_label;
    }

    std::string get_value()
    {
        return m_value;
    }

    void set_value(const std::string in_val)
    {
        m_value = in_val;
    }

    void set_value(const char* in_val)
    {
        m_value = std::string(in_val);
    }

    form_nv_field(const std::string name, const std::string label, const std::string value)
        : m_name(name), m_label(label), m_value(value)
    {
    }
};

class nv_form
{

private:
    int m_field_count = 0;
    FIELD* m_fields[40];
    FORM* m_form;
    std::string m_title;
    std::vector<form_nv_field> m_nv_list;

    /**
* Show Form
*
* @return int
* @throws 
* @exceptsafe yes
*/
    int show_form()
    {
        WINDOW* the_form_win;
        int ch, rows, cols;

        // Create the form and post it
        m_form = new_form(m_fields);

        // Calculate the area required for the form
        scale_form(m_form, &rows, &cols);

        // Create the window to be associated with the form
        the_form_win = newwin(rows + 4, cols + 4, 4, 4);
        keypad(the_form_win, TRUE);

        // Set main window and sub window
        set_form_win(m_form, the_form_win);
        set_form_sub(m_form, derwin(the_form_win, rows, cols, 2, 2));

        // Print a border around the main window and print a title
        box(the_form_win, 0, 0);
        print_in_middle(the_form_win, 1, 0, cols + 4, (char*)m_title.c_str(), COLOR_PAIR(1));
        post_form(m_form);
        wrefresh(the_form_win);
        mvprintw(LINES - 2, 0, "Use UP, DOWN arrow keys to switch between fields");
        refresh();

        bool done = false;

        // Loop through to get user requests
        while (!done)
        {
            ch = wgetch(the_form_win);

            switch (ch)
            {
            case KEY_DOWN:
                // Go to next field
                form_driver(m_form, REQ_NEXT_FIELD);
                // Go to the end of the present buffer
                // Leaves nicely at the last character
                form_driver(m_form, REQ_END_LINE);
                break;

            case KEY_UP:
                // Go to previous field
                form_driver(m_form, REQ_PREV_FIELD);
                form_driver(m_form, REQ_END_LINE);
                break;

            default:
                // If this is a normal character, it gets
                // Printed
                form_driver(m_form, ch);
                break;
            }

            // CR means we are done
            if (10 == ch)
                done = true;
            ;
        }

        auto feelds = read_fields();

        // Un post form and free the memory
        unpost_form(m_form);
        free_form(m_form);
        free_field(m_fields[0]);
        free_field(m_fields[1]);
        endwin();

        return 0;
    }

    /**
* Print in middle
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
* Read the value of a form field
*
* @param[in] const int index
* @return std::string
* @throws 
* @exceptsafe yes
*/
    std::string read_field(const int index)
    {
        form_driver(m_form, REQ_VALIDATION);
        auto field_val = field_buffer(m_fields[index], 0);
        return std::string(field_val);
    }

    /**
* Read the value of all form fields
*
* @param[in] const int index
* @return std::string
* @throws 
* @exceptsafe yes
*/
    std::vector<std::string> read_fields()
    {
        std::vector<std::string> out_feelds;

        for (int index = 0; index < m_field_count; index++)
        {
            out_feelds.push_back(read_field(index));

            //odd indexes are values
            if (index % 2 == 1)
            {
                int ff = index / 2;
                m_nv_list[ff].set_value(read_field(index));
            }
        }

        return out_feelds;
    }

    /**
* Print all form fields
*
* @param[in] std::vector<std::string> feelds
* @throws 
* @exceptsafe yes
*/
    void print_fields(const std::vector<std::string> feelds)
    {
        std::vector<std::string> out_feelds;

        for (std::string feeld : feelds)
        {
            auto cval = feeld.c_str();
            std::cout << cval;
        }
    }

    /**
* Initialize curses
*
* @throws 
* @exceptsafe yes
*/
    void init_curses()
    {
        //_lastInstance = this;

        // Initialize ncurses
        initscr();
        start_color();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);

        // Color pairs
        init_pair(1, COLOR_RED, COLOR_BLACK);
    }

    /**
* Initialize fields
*
* @param[in] std::vector<form_nv_field> fields
* @throws 
* @exceptsafe yes
*/
    void init_fields(const std::vector<form_nv_field> fields)
    {

        m_nv_list = fields;
        int index = 0;
        int row = 1;

        for (auto field : fields)
        {

            // label
            m_fields[index] = new_field(1, 10, row, 1, 0, 0);
            field_opts_off(m_fields[index], O_ACTIVE);
            set_field_buffer(m_fields[index], 0, field.get_label().c_str());

            index++;

            // field
            m_fields[index] = new_field(1, 10, row, 10, 0, 0);
            set_field_back(m_fields[index], A_UNDERLINE);
            field_opts_off(m_fields[index], O_AUTOSKIP);

            index++;
            row++;
        }

        m_fields[index] = NULL;
        m_field_count = index;
    }

public:
    /**
* Initialize
*
* @param[in] std::vector<form_nv_field> fields
* @throws 
* @exceptsafe yes
*/
    void init(const std::string title, const std::vector<form_nv_field> fields)
    {
        m_title = title;
        init_curses();
        init_fields(fields);
    }

    /**
* Get field values
*
* @return std::vector<form_nv_field>
* @throws 
* @exceptsafe yes
*/
    std::vector<form_nv_field> get_fields()
    {
        return m_nv_list;
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
        show_form();
        endwin();
    }

}; // class event_form
