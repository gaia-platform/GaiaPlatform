/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <unistd.h>

#include <iostream>

#include "gaia/common.hpp"
#include "gaia/system.hpp"

#include "gaia_campus.h"

using namespace std;

using namespace gaia::common;
using namespace gaia::campus;

const string c_prompt = "campus> ";

void enter_face_scanning_data()
{
    string building_code;
    string date;
    string time;
    string face_signature;

    cout << "Enter building code: ";
    cin >> building_code;

    cout << "Enter date (YYYY-MM-DD): ";
    cin >> date;

    cout << "Enter time (hhmm): ";
    cin >> time;

    cout << "Enter face signature: ";
    cin >> face_signature;

    int time_as_int = atoi(time.c_str());
    if (time_as_int == 0 && time.compare("0000"))
    {
        cout
            << endl << "Time value '" << time << "' is invalid! Operation aborted!"
            << endl;

        return;
    }

    gaia::db::begin_transaction();

    gaia_id_t building_id = c_invalid_gaia_id;
    for (auto& building : buildings_t::list())
    {
        if (building_code.compare(building.code()) == 0)
        {
            building_id = building.gaia_id();
            break;
        }
    }

    if (building_id == c_invalid_gaia_id)
    {
        cout
            << endl << "Building code '" << building_code << "' is invalid! Operation aborted!"
            << endl;

        gaia::db::rollback_transaction();

        return;
    }

    gaia_id_t face_scan_id = face_scanning_t::insert_row(date.c_str(), time_as_int, face_signature.c_str());

    // Get back face scan record to set its building reference.
    face_scanning_t face_scan = face_scanning_t::get(face_scan_id);
    face_scan.references()[c_parent_face_scanning_building_face_scanning] = building_id;

    gaia::db::commit_transaction();

    cout
        << "Inserted face scanning row with id '" << face_scan_id << "'."
        << endl;

    // Wait for rules to trigger.
    sleep(1);
}

int main()
{
    cout
        << "Campus example is running..."
        << endl << "Enter data with 'e' command, quit application with 'q'."
        << endl;

    gaia::system::initialize();

    string command;
    while (true)
    {
        cout << c_prompt;

        cin >> command;

        if (command.compare("q") == 0)
        {
            break;
        }
        else if (command.compare("e") == 0)
        {
            enter_face_scanning_data();
        }
        else
        {
            cout
                << "Invalid command '" << command << "'! Valid commands are 'e' and 'q'."
                << endl;
        }
    }

    gaia::system::shutdown();

    cout << "Campus example has shut down." << endl;
}
