/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <unistd.h>

#include <cstring>
#include <iostream>

#include "retail_assert.hpp"
#include "gaia_db.hpp"

using namespace gaia::common;

namespace gaia
{
namespace db
{

static const char* SE_SERVER_NAME = "gaia_semock_server";

void start_server() {
    // REVIEW: we should be using a proper process library for this, so we can save the PID.
    char cmd[100];
    int written = snprintf(cmd, sizeof(cmd), "pkill -f -9 %s", SE_SERVER_NAME);
    retail_assert(written > 0 && (size_t)written < sizeof(cmd));
    // Try to kill any existing SE server process.
    cerr << cmd << endl;
    ::system(cmd);
    written = snprintf(cmd, sizeof(cmd), "%s &", SE_SERVER_NAME);
    retail_assert(written > 0 && (size_t)written < sizeof(cmd));
    // Launch SE server in background.
    cerr << cmd << endl;
    ::system(cmd);
    // Wait for server to initialize.
    cerr << "Waiting for server to initialize...";
    sleep(1);
}

void stop_server() {
    // Try to kill the SE server process.
    // REVIEW: we should be using a proper process library for this, so we can kill by PID.
    char cmd[100];
    int written = snprintf(cmd, sizeof(cmd), "pkill -f -9 %s", SE_SERVER_NAME);
    retail_assert(written > 0 && (size_t)written < sizeof(cmd));
    cerr << cmd;
    ::system(cmd);
}

} // db
} // gaia
