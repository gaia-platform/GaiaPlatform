/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include <spdlog/spdlog.h>

#include "gaia/logger.hpp"
#include "gaia/system.hpp"

#include "gaia_hello.h"

using namespace std;

int main()
{
    spdlog::info("Hello spdlog {}", SPDLOG_VERSION);

    gaia::system::initialize();
    gaia_log::app().info("Hello example is running...");

    gaia::db::begin_transaction();
    gaia::hello::names_t::insert_row("Alice");
    gaia::hello::names_t::insert_row("Bob");
    gaia::hello::names_t::insert_row("Charles");
    gaia::db::commit_transaction();

    gaia_log::app().info("Hello example has shut down.");
    gaia::system::shutdown();
}
