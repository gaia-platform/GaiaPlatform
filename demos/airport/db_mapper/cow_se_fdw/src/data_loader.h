/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

// #include "PerfTimer.h"
#include <cassert>
#include <map>
#include "flatbuffers/flatbuffers.h"
#include "airport_generated.h" // include both flatbuffer types and object API for testing
#include "airport_types.h"
#include "cow_se.h"
#include <fstream>
using namespace std;
using namespace gaia_se;
using namespace gaia::airport;

// provide an option for shared memory file name
// have the client

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

istream& operator>>(istream& str, CSVRow& data)
{
    data.read_next(str);
    return str;
}


class AirportData
{
    public:
        bool init(const char * data_path, bool reset)
        {
            gaia_mem_base::init(reset);
            _data_path = data_path;
            if (_data_path.back() != '/')
            {
                _data_path.push_back('/');
            }
            load();
            check();
            return true;
        }

        bool load()
        {
            load_airlines();
            load_airports();
            load_routes();
            return true;
        }

        // void return type is fine

        bool _load(const char * filename,
            function<bool (flatbuffers::FlatBufferBuilder&, CSVRow &)> loader)
        {
            flatbuffers::FlatBufferBuilder b(128);
            ifstream f(filename);
            if (f.fail()) {
                printf("Could not open '%s'\n", filename);
                return false;
            }

            CSVRow row;
            uint32_t rows = 0;
            uint32_t bad_rows = 0;

            // int64_t ns;
            // PerfTimer(ns, [&]() {
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
                if (loader(b, row)) {
                    rows++;
                } else {
                    bad_rows++;
                }
                b.Clear();
                get_next_id();
            }
            commit_transaction();
            printf("Added %d rows from %s with %d bad rows\n", rows, filename, bad_rows);
            // });
            // printf("Added %d rows from %s in %.2f ms\n", rows, filename, PerfTimer::ns_ms(ns));
            return true;
        }

        bool load_airlines()
        {
            _load(string(_data_path).append("airlines.dat").c_str(),
                [&](flatbuffers::FlatBufferBuilder& b, CSVRow& row)
                {
                    // cannot have a null here
                    int32_t al_id = stoi(row[0].col);
                    // raw flatbuffer API
                    b.Finish(CreateairlinesDirect(b, _node_id,
                        al_id, // al_id
                        row[1].is_null ? 0 : row[1].col.c_str(), // name
                        row[2].is_null ? 0 : row[2].col.c_str(), // alias
                        row[3].is_null ? 0 : row[3].col.c_str(), // iata
                        row[4].is_null ? 0 : row[4].col.c_str(), // icao
                        row[5].is_null ? 0 : row[5].col.c_str(), // callsign
                        row[6].is_null ? 0 : row[6].col.c_str(), // country
                        row[7].is_null ? 0 : row[7].col.c_str() // active
                    ));
                    gaia_se_node::create(_node_id, airport_demo_types::kAirlinesType, b.GetSize(), b.GetBufferPointer());
                    _mapAirlines.insert(pair<int32_t, int64_t>(al_id, _node_id));
                    return true;
                }
            );

            return true;
        }

        bool load_airports()
        {
            _load(string(_data_path).append("airports.dat").c_str(),
                [&](flatbuffers::FlatBufferBuilder& b, CSVRow& row)
                {
                    // cannot have a null here
                    int32_t ap_id = stoi(row[0].col);
                    // raw flatbuffer API
                    b.Finish(CreateairportsDirect(b, _node_id,
                        ap_id, // api_id
                        row[1].is_null ? 0 : row[1].col.c_str(), // name
                        row[2].is_null ? 0 : row[2].col.c_str(), // city
                        row[3].is_null ? 0 : row[3].col.c_str(), // country
                        row[4].is_null ? 0 : row[4].col.c_str(), // iata
                        row[5].is_null ? 0 : row[5].col.c_str(), // icao
                        row[6].is_null ? 0 : stod(row[6].col), // latitude
                        row[7].is_null ? 0 : stod(row[7].col), // longitude
                        row[8].is_null ? 0 : stoi(row[8].col), // altitude
                        row[9].is_null ? 0 : stof(row[9].col), // timezone
                        row[10].is_null ? 0 : row[10].col.c_str(), // dst
                        row[11].is_null ? 0 : row[11].col.c_str(), // tztxt
                        row[12].is_null ? 0 : row[12].col.c_str(), // type
                        row[13].is_null ? 0 : row[13].col.c_str() // source
                    ));
                    gaia_se_node::create(_node_id, airport_demo_types::kAirportsType, b.GetSize(), b.GetBufferPointer());
                    _mapAirports.insert(pair<int32_t, int64_t>(ap_id, _node_id));
                    return true;
                }
            );

            return true;
        }

        bool load_routes()
        {
                // The routes get created as edges between the source
                // and destination airports.
                _load(string(_data_path).append("routes.dat").c_str(),
                [&](flatbuffers::FlatBufferBuilder& b, CSVRow& row)
                {
                    bool ret = true;

                    int32_t src_ap_id = 0;
                    int32_t dst_ap_id = 0;
                    int32_t al_id = 0;

                    if (!row[1].is_null) {
                        al_id = stoi(row[1].col);
                    }

                    if (!row[3].is_null) {
                        src_ap_id = stoi(row[3].col);
                    }

                    if (!row[5].is_null){
                        dst_ap_id = stoi(row[5].col);
                    }

                    int64_t gaia_src_id = _mapAirports[src_ap_id];
                    int64_t gaia_dst_id = _mapAirports[dst_ap_id];
                    int64_t gaia_al_id = _mapAirlines[al_id];

                    if (gaia_al_id == 0 || gaia_src_id == 0 || gaia_dst_id == 0)
                    {
                        // skip the row if we don't have the airport data
                        // in the data set

                        // if (gaia_src_id == 0) {
                        //     printf("skipping route for unfound src_ap '%s' in dataset\n",
                        //         row[2].col.c_str());
                        // }

                        // if (gaia_dst_id == 0) {
                        //     printf("skipping route for unfound dst_ap '%s' in dataset\n",
                        //         row[4].col.c_str());
                        // }
                        ret = false;
                    }
                    else
                    {
                        // raw flatbuffer API
                        b.Finish(CreateroutesDirect(b, _node_id, gaia_al_id, gaia_src_id, gaia_dst_id,
                            row[0].is_null ? 0 : row[0].col.c_str(), // airline
                            al_id, // al_id
                            row[2].is_null ? 0 : row[2].col.c_str(), // src_ap
                            src_ap_id, // src_ap_id
                            row[4].is_null ? 0 : row[4].col.c_str(), // dst_ap
                            dst_ap_id, // dst_ap_id
                            row[6].is_null ? 0 : row[6].col.c_str(), // codeshare
                            row[7].is_null ? 0 : stoi(row[7].col), // stops
                            row[8].is_null ? 0 : row[8].col.c_str() // equipment
                        ));
                        gaia_se_edge::create(_node_id, airport_demo_types::kRoutesType, gaia_src_id, gaia_dst_id,
                            b.GetSize(), b.GetBufferPointer());
                    }
                    return ret;
                }
            );

            return true;
        }

        bool check()
        {
            begin_transaction();
            _check_airlines();
            _check_airports();
            _check_routes();
            commit_transaction();
            return true;
        }

        void _check_airlines()
        {
            auto first_airline_node = gaia_ptr<gaia_se_node>::find_first(airport_demo_types::kAirlinesType);
            const airlines *airline = flatbuffers::GetRoot<airlines>(first_airline_node->payload);
            printf("first airline object: gaia_id=%ld, al_id=%d, name=%s\n",
                airline->gaia_id(), airline->al_id(), airline->name()->c_str());
        }

        void _check_airports()
        {
            auto first_airport_node = gaia_ptr<gaia_se_node>::find_first(airport_demo_types::kAirportsType);
            const airports *airport = flatbuffers::GetRoot<airports>(first_airport_node->payload);
            printf("first airport object: gaia_id=%ld, ap_id=%d, name=%s\n",
                airport->gaia_id(), airport->ap_id(), airport->name()->c_str());
        }

        void _check_routes()
        {
            auto first_route_node = gaia_ptr<gaia_se_edge>::find_first(airport_demo_types::kRoutesType);
            const routes *route = flatbuffers::GetRoot<routes>(first_route_node->payload);
            printf("first route object: gaia_id=%ld, airline=%s, src_ap=%s, dst_ap=%s\n",
                route->gaia_id(), route->airline()->c_str(), route->src_ap()->c_str(), route->dst_ap()->c_str());
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
        std::map<int32_t, int64_t> _mapAirports;
        std::map<int32_t, int64_t> _mapAirlines;
};
