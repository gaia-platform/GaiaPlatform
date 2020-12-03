/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "performance_timer.hpp"
#include <cassert>
#include <map>
#include <fstream>
#include "gaia/direct_access/nullable_string.hpp"
#include "airport_q1_gaia_generated.h" // include both flatbuffer types and object API for testing 
using namespace std;
using namespace gaia::db;
using namespace gaia::airport;

// provide an option for shared memory file name
// have the client

class csv_row_t
{
    public:
        struct csv_col_t
        {
            csv_col_t(const string& col, bool is_null) : col(col), is_null(is_null) {}
            bool is_null;
            string col;
        };

        csv_row_t(){};
        csv_col_t const& operator[](size_t index) const
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
                    _row.push_back(csv_col_t(token, is_null));
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
            _row.push_back(csv_col_t(token, is_null));
        }

    private:
        vector<csv_col_t> _row;
};

istream& operator>>(istream& str, csv_row_t& data)
{
    data.read_next(str);
    return str;
}


class AirportData
{
    public:
        bool init(const char * data_path)
        {
            gaia_mem_base::init(true);
            _data_path = data_path;
            if (_data_path.back() != '/')
            {
                _data_path.push_back('/');
            }
            load();

            return true;
        }

        bool load()
        {
            get_next_id();
            load_airlines();
            load_airports();
            load_routes();
            return true;
        }

        // void return type is fine

        bool _load(const char * filename, 
            function<void (csv_row_t &)> loader)
        {
            ifstream f(filename);
            if (f.fail()) {
                printf("Could not open '%s'\n", filename);
                return false;
            }
            
            csv_row_t row;
            uint32_t rows = 0;

            int64_t ns;
            performance_timer(ns, [&]() {
                begin_transaction();
                while (f >> row)
                {
                    // set this value and put a breakpoint on the assert below
                    // to debug parsing a specific row
                    static uint32_t dbg_row = 6666;
                    if (rows == dbg_row)
                    {   
                        assert(rows == dbg_row);
                    }
                    loader(row);
                    get_next_id();
                    rows++;
                }
                commit_transaction();
            });
            printf("Added %d rows from %s in %.2f ms\n", rows, filename, performance_timer::ns_ms(ns));
            return true;
        }

        bool load_airlines()
        {
            _load(string(_data_path).append("airlines.dat").c_str(),
                [&](csv_row_t& row)
                {
                    // cannot have a null here
                    int32_t al_id = stoi(row[0].col);
                    auto al_gaia_id = Airlines::insert_row(_node_id,
                        al_id,
                        row[1].is_null ? 0 : row[1].col.c_str(), // name
                        row[2].is_null ? 0 : row[2].col.c_str(), // alias
                        row[3].is_null ? 0 : row[3].col.c_str(), // iata
                        row[4].is_null ? 0 : row[4].col.c_str(), // icao
                        row[5].is_null ? 0 : row[5].col.c_str(), // callsign
                        row[6].is_null ? 0 : row[6].col.c_str(), // country
                        row[7].is_null ? 0 : row[7].col.c_str()  // active
                    );
                    auto al = Airlines::get_row_by_id(al_gaia_id);
                    _mapAirlines.insert(pair<int32_t, gaia_id_t>(al_id, al->gaia_id()));
                }
            );

            return true;
        }

        bool load_airports()
        {
            _load(string(_data_path).append("airports.dat").c_str(), 
                [&](csv_row_t& row)
                {
                    // cannot have a null here
                    int32_t ap_id = stoi(row[0].col);
                    // raw flatbuffer API
                    auto ap_gaia_id = Airports::insert_row(_node_id,
                        ap_id, // api_id
                        row[1].is_null ? 0 : row[1].col.c_str(), // name
                        row[2].is_null ? 0 : row[2].col.c_str(), // city
                        row[3].is_null ? 0 : row[3].col.c_str(), // country
                        row[4].is_null ? 0 : row[4].col.c_str(), // iata
                        row[5].is_null ? 0 : row[5].col.c_str(), // icao
                        row[6].is_null ? 0 : stod(row[6].col), // latitude
                        row[7].is_null  ? 0 : stod(row[7].col), // longitude
                        row[8].is_null  ? 0 : stoi(row[8].col), // altitude
                        row[9].is_null  ? 0 : stof(row[9].col), // timezone
                        row[10].is_null ? 0 : row[10].col.c_str(), // dst
                        row[11].is_null ? 0 : row[11].col.c_str(), // tztxt
                        row[12].is_null ? 0 : row[12].col.c_str(), // type
                        row[13].is_null ? 0 : row[13].col.c_str() // source
                    );
                    auto ap = Airports::get_row_by_id(ap_gaia_id);
                    _mapAirports.insert(pair<int32_t, gaia_id_t>(ap_id, ap->gaia_id()));
                }
            );

            return true;
        }

        bool load_routes()
        {
                // The routes get created as edges between the source
                // and destination airports.
                _load(string(_data_path).append("routes.dat").c_str(), 
                [&](csv_row_t& row)
                {
                    int32_t src_ap_id = 0;
                    int32_t dst_ap_id = 0;
                    int32_t al_id = 0;

                    if (!row[1].is_null) {
                        al_id = stoi(row[1].col);
                    }

                    if (!row[3].is_null) {
                        src_ap_id = stoi(row[3].col);
                    }

                    if (!row[5].is_null) {
                        dst_ap_id = stoi(row[5].col);
                    }

                    gaia_id_t gaia_src_id = _mapAirports[src_ap_id];
                    gaia_id_t gaia_dst_id = _mapAirports[dst_ap_id];
                    gaia_id_t gaia_al_id = _mapAirlines[al_id];

                    if (gaia_src_id != 0 && gaia_dst_id != 0)
                    {
                        flatbuffers::FlatBufferBuilder b(128);
                        // raw flatbuffer API
                        b.Finish(CreateroutesDirect(b, _node_id,
                            gaia_al_id, gaia_src_id, gaia_dst_id,
                            row[0].is_null ? 0 : row[0].col.c_str(), // airline
                            al_id,                                   // al_id
                            row[2].is_null ? 0 : row[2].col.c_str(), // src_ap
                            src_ap_id,                               // src_ap_id
                            row[4].is_null ? 0 : row[4].col.c_str(), // dst_ap
                            dst_ap_id,                               // dst_ap_id
                            row[6].is_null ? 0 : row[6].col.c_str(), // codeshare
                            row[7].is_null ? 0 : stoi(row[7].col),   // stops
                            row[8].is_null ? 0 : row[8].col.c_str()  // equipment
                        ));
                        gaia_se_edge::create(_node_id, gaia::airport::kRoutesType, gaia_src_id, gaia_dst_id,
                            b.GetSize(), b.GetBufferPointer());
                    }
                }
            );

            return true;
        }

        gaia_id_t get_next_id()
        {
            return ++_node_id;
        }

    private:
        // node ids
        gaia_id_t _node_id = 0;
        std::string _data_path;
        // map from db ids to gaia ids
        std::map<int32_t, gaia_id_t> _mapAirports;
        std::map<int32_t, gaia_id_t> _mapAirlines;
};
