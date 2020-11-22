/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "performance_timer.hpp"
#include <cassert>
#include <fstream>
#include <unordered_set>
#include "gaia/direct_access/nullable_string.hpp"
#include "airport_q1_gaia_generated.h"
using namespace std;
using namespace gaia::db;
using namespace gaia::airport;

// Query #1: Find the 1 stop connection airports between [SEA] and [OTP].
// Query #4: Which cities are visited from SEA to an airport that flies Tarom to OTP.
int32_t query_change_airport_SEA_to_OTP(vector<string> req, std::function<void (Airports* ap)> fn)
{
    unordered_set<Airports*> airports;
    uint32_t i = 0;

    string start_airport("SEA");
    if (req.size() >= 2 && req[1].size() > 0) {
        start_airport = req[1];
    }
    string dest_airport("OTP");
    if (req.size() >= 3 && req[2].size() > 0) {
        dest_airport = req[2];
    }
    printf("[%s][%s]\n", start_airport.c_str(), dest_airport.c_str());

    begin_transaction();
    Airports* ap = Airports::get_first();
    for(; ap; ap = ap->get_next()) {
        if (ap->iata() != nullptr && strcmp(start_airport.c_str(), ap->iata()) == 0) {
            // Find each airport this one has flights to.
            for (auto next_ap = ap->get_first_node(gaia::airport::kRoutesType, Airports::edge_orientation_t::to);
                next_ap;
                next_ap = ap->get_next_node())
            {
                for (auto final_ap = next_ap->get_first_node(gaia::airport::kRoutesType, Airports::edge_orientation_t::to);
                    final_ap;
                    final_ap = next_ap->get_next_node())
                {
                    if (final_ap->iata() != nullptr && strcmp(dest_airport.c_str(), final_ap->iata()) == 0) {
                        // add the connecting airport to the vector
                        airports.insert(next_ap);
                    }
                }
            }
        }
    }
    for (auto it = airports.begin(); it != airports.end(); ++it) {
        fn(*it);
        i++;
    }
    commit_transaction();
    return i;
}

// Query #2:  Describe the 1 layover routes between [SEA] and [OTP].
int32_t query_SEA_to_OTP_1_layover(vector<string>& req)
{
    uint32_t i = 0;

    string start_airport("SEA");
    if (req.size() >= 2 && req[1].size() > 0) {
        start_airport = req[1];
    }
    string dest_airport("OTP");
    if (req.size() >= 3 && req[2].size() > 0) {
        dest_airport = req[2];
    }
    printf("[%s][%s]\n", start_airport.c_str(), dest_airport.c_str());

    begin_transaction();
    Airports* ap = Airports::get_first();
    for(; ap; ap = ap->get_next()) {
        if (ap->iata() != nullptr && strcmp(start_airport.c_str(), ap->iata()) == 0) {
            // Find each airport this one has flights to.
            for (auto next_ap = ap->get_first_node(gaia::airport::kRoutesType, Airports::edge_orientation_t::to);
                next_ap;
                next_ap = ap->get_next_node())
            {
                for (auto final_ap = next_ap->get_first_node(gaia::airport::kRoutesType, Airports::edge_orientation_t::to);
                    final_ap;
                    final_ap = next_ap->get_next_node())
                {
                    if (final_ap->iata() != nullptr && strcmp(dest_airport.c_str(), final_ap->iata()) == 0) {
                        auto ap_edge_id = ap->gaia_edge_id();
                        auto ap_edge = Routes::get_edge_by_id(ap_edge_id);
                        auto next_ap_edge_id = next_ap->gaia_edge_id();
                        auto next_ap_edge = Routes::get_edge_by_id(next_ap_edge_id);
                        printf("==>%s on %s to %s on %s to %s\n", ap->iata(), ap_edge->airline(),
                            next_ap->iata(), next_ap_edge->airline(), final_ap->iata());
                        i++;
                    }
                }
            }
        }
    }
    commit_transaction();
    return i;
}

// Query #3: Which airlines go from [SEA] to an airport that flies to [OTP] using Tarom [RO].
int32_t query_airline_to_Tarom_connection(vector<string> req)
{
    unordered_set<string> routes;
    uint32_t i = 0;
    string start_airport("SEA");

    if (req.size() >= 2 && req[1].size() > 0) {
        start_airport = req[1];
    }
    string dest_airport("OTP");
    if (req.size() >= 3 && req[2].size() > 0) {
        dest_airport = req[2];
    }
    string airline("RO");
    if (req.size() >= 4 && req[3].size() > 0) {
        airline = req[3];
    }
    printf("[%s][%s][%s]\n", start_airport.c_str(), dest_airport.c_str(), airline.c_str());

    begin_transaction();
    Airports* ap = Airports::get_first();
    for(; ap; ap = ap->get_next()) {
        if (ap->iata() != nullptr && strcmp(start_airport.c_str(), ap->iata()) == 0) {
            // Find each airport this one has flights to.
            for (auto next_ap = ap->get_first_node(gaia::airport::kRoutesType, Airports::edge_orientation_t::to);
                next_ap;
                next_ap = ap->get_next_node())
            {
                for (auto final_ap = next_ap->get_first_node(gaia::airport::kRoutesType, Airports::edge_orientation_t::to);
                    final_ap;
                    final_ap = next_ap->get_next_node())
                {
                    if (final_ap->iata() != nullptr && strcmp(dest_airport.c_str(),final_ap->iata()) == 0) {
                        // One final check - is the last leg serviced by Tarom (RO) airline?
                        auto next_ap_edge_id = next_ap->gaia_edge_id();
                        auto next_ap_edge = Routes::get_edge_by_id(next_ap_edge_id);
                        if (next_ap_edge->airline() && strcmp(airline.c_str(), next_ap_edge->airline()) == 0) {
                            // add the previous airline to the vector
                            auto ap_edge_id = ap->gaia_edge_id();
                            auto ap_edge = Routes::get_edge_by_id(ap_edge_id);
                            routes.insert(string(ap_edge->airline()));
                        }
                    }
                }
            }
        }
    }
    for (auto it = routes.begin(); it != routes.end(); ++it) {
        printf("==>%s\n", (*it).c_str());
        i++;
    }
    commit_transaction();
    return i;
}

// Query #5: Describe the 1 layover routes between SEA and OTP that avoid Tarom to OTP.
int32_t query_SEA_to_OTP_no_Tarom()
{
    uint32_t i = 0;
    begin_transaction();
    Airports* ap = Airports::get_first();
    for(; ap; ap = ap->get_next()) {
        if (ap->iata() != nullptr && strcmp("SEA", ap->iata()) == 0) {
            // Find each airport this one has flights to.
            for (auto next_ap = ap->get_first_node(gaia::airport::kRoutesType, Airports::edge_orientation_t::to);
                next_ap;
                next_ap = ap->get_next_node())
            {
                for (auto final_ap = next_ap->get_first_node(gaia::airport::kRoutesType, Airports::edge_orientation_t::to);
                    final_ap;
                    final_ap = next_ap->get_next_node())
                {
                    if (final_ap->iata() != nullptr && strcmp("OTP", final_ap->iata()) == 0) {
                        auto next_ap_edge_id = next_ap->gaia_edge_id();
                        auto next_ap_edge = Routes::get_edge_by_id(next_ap_edge_id);
                            auto ap_edge_id = ap->gaia_edge_id();
                            auto ap_edge = Routes::get_edge_by_id(ap_edge_id);
                        if (strcmp(next_ap_edge->airline(), "RO") != 0) {
                            printf("==>%s on %s to %s on %s to %s\n", ap->iata(), ap_edge->airline(),
                                next_ap->iata(), next_ap_edge->airline(), final_ap->iata());
                            i++;
                        }
                    }
                }
            }
            break;
        }
    }
    commit_transaction();
    return i;
}

int main ()
{
    int64_t ns;
    int64_t total_ns = 0;
    int32_t rows;
    string request;
    char start[4];
    char dest[4];
    char airline_of_interest[3];

    gaia_mem_base::init(false);

    for (;;) {
        printf("\n1. Find the 1 stop connection airports between [SEA] and [OTP].\n");
        printf("2. Describe the 1 layover routes between [SEA] and [OTP].\n");
        printf("3. Which airlines go from [SEA] to an airport that flies to [OTP] using Tarom [RO].\n");
        printf("4. Which cities are visited from SEA to an airport that flies Tarom to OTP.\n");
        printf("5. Describe the 1 layover routes between SEA and OTP that avoid Tarom to OTP.\n");
        printf("q. Quit\n");
        printf("> ");
        // fgets(request, sizeof(request), stdin);
        cin >> request;
        vector<string> req;
        stringstream req_stream(request);
        while (req_stream.good()) {
            string substr;
            getline(req_stream, substr, ',');
            req.push_back(substr);
        }
        switch (req[0][0]) {
            case '1':
            performance_timer(ns, [&]() {
                rows = query_change_airport_SEA_to_OTP(req, [&](Airports* ap) {
                    printf("==>%s\n", ap->name());
                });
            });
            printf("query_change_airport_SEA_to_OTP, airport name: produced %u rows in %.2f msec\n", rows, performance_timer::ns_ms(ns));
            total_ns += ns;
            break;

            case '2':
            performance_timer(ns, [&]() {
                rows = query_SEA_to_OTP_1_layover(req);
            });
            printf("query_SEA_to_OTP_1_layover: produced %u rows in %.2f msec\n", rows, performance_timer::ns_ms(ns));
            total_ns += ns;
            break;

            case '3':
            performance_timer(ns, [&]() {
                rows = query_airline_to_Tarom_connection(req);
            });
            printf("query_airline_to_Tarom_connection: produced %u rows in %.2f msec\n", rows, performance_timer::ns_ms(ns));
            total_ns += ns;
            break;
            
            case '4':
            performance_timer(ns, [&]() {
                rows = query_change_airport_SEA_to_OTP(req, [&](Airports* ap) {
                    printf("==>%s\n", ap->city());
                });
            });
            printf("query_change_airport_SEA_to_OTP, airport city: produced %u rows in %.2f msec\n", rows, performance_timer::ns_ms(ns));
            total_ns += ns;
            break;

            case '5':
            performance_timer(ns, [&]() {
                rows = query_SEA_to_OTP_no_Tarom();
            });
            printf("query_SEA_to_OTP_no_Tarom: produced %u rows in %.2f msec\n", rows, performance_timer::ns_ms(ns));
            total_ns += ns;
            break;

            case 'q':
            printf("completed, total query time %.2f\n", performance_timer::ns_ms(total_ns));
            exit(0);

            default:
            printf("INVALID REQUEST\n");
            break;
        }
    }

    printf("TOTAL TIME: %.2f msec\n", performance_timer::ns_ms(total_ns));
}
