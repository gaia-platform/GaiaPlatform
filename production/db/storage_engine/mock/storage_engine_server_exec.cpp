/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "retail_assert.hpp"
#include "storage_engine_server.hpp"

int main(int argc, char* argv[]) {
    // Allow test code to set an arbitrary name for the server's listening socket.
    if (argc > 1) {
        gaia::common::retail_assert(argc == 2, "You can only specify the name of the server listening socket in main().");
        gaia::db::server::set_server_socket_name(argv[1]);
    }
    gaia::db::server::run();
}
