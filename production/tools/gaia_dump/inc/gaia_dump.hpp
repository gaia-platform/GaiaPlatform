/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////

#pragma once

#include <iostream>

#include "gaia_ptr.hpp"

using namespace std;

typedef vector<gaia_id_t> type_vector;

string gaia_dump(gaia_id_t, gaia_id_t, bool, bool, bool, int&, type_vector);
