/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cassert>
#include <map>
#include <fstream>
#include "gaia_gaia_u.h" // include both flatbuffer types and object API for testing 

using namespace std;
using namespace gaia::db;
using namespace gaia::gaia_u;

// provide an option for shared memory file name
// have the client

// Simplify to just use getline and string find...
// getline does have a delimter but we want both \n and ,
// Check in sources somewhere!

// Weekend goals:

// Parse file input
// display in form with tree controls
// Python binding layer
// Create a registration form!

class gaia_u_loader_t
{
public:
    bool load(const char* data_file);

private:
    typedef std::vector<std::string> row_t;

    void load_Persons(row_t& row);
    void load_Buildings(row_t& row);
    void load_Rooms(row_t& row);
    void load_Staff(row_t& row);
    void load_Events(row_t& row);
    void load_Students(row_t& row);
    void load_Parents(row_t& row);
    void load_Restrictions(row_t& row);

private:
    std::map<uint32_t, gaia::common::gaia_id_t> m_persons_ids;
    std::map<uint32_t, gaia::common::gaia_id_t> m_students_ids;
    std::map<uint32_t, gaia::common::gaia_id_t> m_parents_ids;
    std::map<uint32_t, gaia::common::gaia_id_t> m_buildings_ids;
    std::map<uint32_t, gaia::common::gaia_id_t> m_staff_ids;
    std::map<uint32_t, gaia::common::gaia_id_t> m_rooms_ids;
    std::map<uint32_t, gaia::common::gaia_id_t> m_events_ids;
};
