/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

class CSVRow
{
    public:
        struct CSVCol
        {
            CSVCol(const string& col, bool is_null) : col(col), is_null(is_null) {}
            bool is_null;
            string col;
        };

        CSVRow(){};
        CSVCol const& operator[](size_t index) const
        {
            return _row[index];
        }

        void read_next(istream& str)
        {
            string line;
            getline(str, line);
            _row.clear();
            parse_line(line);
        }

        // parse a comma delimited string into a row
        // handle the following cases:
        // 5, "a comma, embedded", "yay"
        // 10, "some ""quotes"" here"
        // 15, \N, "well, that was a null"
        void parse_line(string line)
        {
            std::string token;
            auto it = line.begin();
            bool in_quote = false;
            bool is_null = false;

            while (it != line.end())
            {
                char c = *it++;
                if (c == '\"')
                {
                    in_quote = !in_quote;
                }
                else
                if (c == '\\')
                {
                    char esc_c = *it++;
                    if ('N' == esc_c)
                    {
                        // this column is null
                        if (!in_quote)
                        {
                            is_null = true;
                        }
                    }
                    else // hack for airline data
                    if ('\\' == esc_c)
                    {
                        char a = *it++;

                        // unexpected esc sequence in the data
                        // please provide a case for this
                        token.push_back(a);
                    }
                    else
                    if ('\'' == esc_c)
                    {
                        token.push_back(esc_c);
                    }
                    else
                    {
                        assert(0); // unhandled escape sequence
                    }
                }
                else
                if (c == ',' && !in_quote)
                {
                    _row.push_back(CSVCol(token, is_null));
                    token.clear();
                    is_null = false;
                }
                else
                if (c == '\r')
                {
                    continue;
                }
                else
                {
                    token.push_back(c);
                }
            }
            // get our last column
            _row.push_back(CSVCol(token, is_null));
        }

    private:
        vector<CSVCol> _row;
};
