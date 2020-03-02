/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cstring>
#include <cstdint>
#include <list>
#include <map>
#include "airport_generated.h" // include both flatbuffer types and object API for testing 
#include "GaiaObj.h"
#include "AirportTypes.h"
#include "NullableString.h"
#include "PerfTimer.h"

using namespace std;
using namespace gaia_se;
using namespace AirportDemo;

map<string, gaia_id_t> country_map;

struct Airport : public GaiaObj<AirportDemo::kAirportsType, Airport, airports, airportsT>
{
    #define get(field) (_copy ? (_copy->field) : (_fb->field()))
    #define get_str(field) (_copy ? _copy->field.c_str() : _fb->field() ? _fb->field()->c_str() : nullptr)
    #define set(field, value) (copy_write()->field = value)

    int64_t Gaia_id() const { return get(Gaia_id); }
    int64_t Gaia_Next_id() const { return get(Gaia_Next_id); }
    int64_t Gaia_Prev_id() const { return get(Gaia_Prev_id); }
    int32_t ap_id() const { return get(ap_id); }
    const char * name() const { return get_str(name); }
    const char * city() const { return get_str(city); }
    const char * country() const { return get_str(country); }
    const char * iata() const { return get_str(iata); }
    const char * icao() const { return get_str(icao); }
    double latitude() const { return get(latitude); }
    double longitude() const { return get(longitude); }
    int32_t altitude() const { return get(altitude); }
    float timezone() const { return get(timezone); }
    const char * dst() const { return get_str(dst); }
    const char * tztext() const { return get_str(tztext); }
    const char * type() const { return get_str(type); }
    const char * source() const { return get_str(source); }
    void set_Gaia_id(uint64_t i) { set(Gaia_id, i); }
    void set_Gaia_Next_id(uint64_t i) { set(Gaia_Next_id, i); }
    void set_Gaia_Prev_id(uint64_t i) { set(Gaia_Prev_id, i); }
    void set_ap_id(uint32_t i) { set(ap_id, i); }
    void set_name(const char * s) { set(name, s); }
    void set_city(const char * s) { set(city, s); }
    void set_country(const char * s) { set(country, s); }
    void set_iata(const char * s) { set(iata, s); }
    void set_icao(const char * s) { set(icao, s); }
    void set_latitude(double d) { set(latitude, d); }
    void set_longitude(double d) { set(longitude, d); }
    void set_altitude(int32_t i) { set(altitude, i); }
}; //Airport 

class GaiaTx
{
public:
    GaiaTx(std::function<bool ()> fn) {
        gaia_se::begin_transaction();
        if (fn()) {
            gaia_se::commit_transaction();
        }
        else {
            gaia_se::rollback_transaction();
        }
    }
};

uint32_t traverse_list_gaia()
{
    uint32_t i = 0;
    GaiaTx([&]() {
        Airport * ap = Airport::GetFirst();
        for(; ap; ap = ap->GetNext()) {
            int32_t ap_id = ap->ap_id();
            ap_id++;
            if (ap->iata()){
                auto len = strlen(ap->iata());
            }
            i++;
        }
        return true;
    });
    return i;
}

uint32_t build_country_map(bool print, int32_t* countries)
{
    uint32_t i = 0;
    *countries = 0;
    GaiaTx([&]() {
        Airport * ap = Airport::GetFirst();
        for(; ap; ap = ap->GetNext()) {
            auto id = ap->Gaia_id();
            auto head_id = country_map[ap->country()];
            if (head_id != 0) {
                // new row becomes new head
                auto head_ap = Airport::GetRowById(head_id);
                head_ap->set_Gaia_Prev_id(id);
                ap->set_Gaia_Next_id(head_id);
                ap->Update();
                head_ap->Update();
            }
            else {
                if (print)
                    printf("New country: %s\n", ap->country());
                (*countries)++;
            }
            country_map[ap->country()] = id;
            i++;
    }
        return true;
    });
    return i;
}

uint32_t traverse_country_map(bool print, int32_t* countries)
{
    uint32_t i = 0;
    *countries = 0;
    GaiaTx([&]() {
        for (map<string, gaia_id_t>::iterator it = country_map.begin();
             it != country_map.end();
             ++it)
        {
            if (print)
                printf("Country Airports for %s\n", it->first.c_str());
            auto head_id = it->second;
            Airport * ap;
            for (ap = Airport::GetRowById(head_id);
                 ap;
                 ap = Airport::GetRowById(ap->Gaia_Next_id()))
            {
                auto ap_id = ap->Gaia_id();
                ap_id++;
                i++;
                if (print) {
                    printf("%016lx %016lx %016lx %2d %s (%s)\n", ap->Gaia_id(), ap->Gaia_Next_id(),
                        ap->Gaia_Prev_id(), ap->ap_id(), ap->name(), ap->country());
                }
            }
            (*countries)++;
        }
        return true;
    });
    return i;
}

uint32_t traverse_links_list_gaia(bool print)
{
    uint32_t i = 0;
    GaiaTx([&]() {
        Airport * ap = Airport::GetFirst();
        for(; ap; ap = Airport::GetRowById(ap->Gaia_Next_id())) {
            int32_t ap_id = ap->ap_id();
            ap_id++;
            if (ap->iata()){
                auto len = strlen(ap->iata());
            }
            if (print)
                printf("%016lx %016lx %016lx %2d %s\n", ap->Gaia_id(), ap->Gaia_Next_id(),
                    ap->Gaia_Prev_id(), ap_id, ap->name());
            i++;
        }
        return true;
    });
    return i;
}

uint32_t update_links()
{
    uint32_t i = 0;

    GaiaTx([&]() 
    {
        gaia_id_t this_id;
        gaia_id_t prev_id = 0;
        Airport * prev_ap = nullptr;
        Airport * ap = Airport::GetFirst();
        for (; ap ; ap = ap->GetNext()) {
            this_id = ap->Gaia_id();
            ap->set_Gaia_Prev_id(prev_id);
            ap->set_Gaia_Next_id(0);
            ap->Update();
            if (prev_ap != nullptr) {
                prev_ap->set_Gaia_Next_id(this_id);
                prev_ap->Update();
            }
            prev_ap = ap;
            prev_id = this_id;
            i++;
        }
        return true;
    });
    return i;
}

uint32_t update_list_gaia()
{
    uint32_t i = 0;

    GaiaTx([&]() 
    {
        Airport * ap = Airport::GetFirst();
        for (; ap ; ap = ap->GetNext()) {
            ap->set_ap_id(ap->ap_id() + 10000);
            ap->Update();
            i++;
        }
        return true;
    });
    return i;
}

uint32_t clear_links()
{
    uint32_t i = 0;

    GaiaTx([&]() 
    {
        Airport * ap = Airport::GetFirst();
        for (; ap ; ap = ap->GetNext()) {
            ap->set_Gaia_Next_id(0);
            ap->set_Gaia_Prev_id(0);
            ap->Update();
            i++;
        }
        return true;
    });
    return i;
}

void print_airports(const char* msg, int count)
{
    GaiaTx([&]() {
        printf("===%s===\n", msg);
        Airport * ap = Airport::GetFirst();
        for(int i=0; i<count && ap; i++, ap=ap->GetNext()) {
            int32_t ap_id = ap->ap_id();
            printf("%016lx %016lx %016lx %2d %s\n", ap->Gaia_id(), ap->Gaia_Next_id(),
                ap->Gaia_Prev_id(), ap_id, ap->name());
        }
        return true;
    });
}

int main (int argc, const char ** argv)
{
    (void)argv;
    bool print = argc > 1;
    gaia_mem_base::init(false);

    int64_t ns;
    int32_t rows;
    int32_t countries;

    // *************************************************
    // read tests
    // *************************************************

    PerfTimer(ns, [&]() {
        rows = traverse_list_gaia();
    });
    printf("traverse_list_gaia: read %u rows at %.0f rows/sec\n", rows, rows/PerfTimer::ns_s(ns));

    PerfTimer(ns, [&]() {
        rows = traverse_list_gaia();
    });
    printf("traverse_list_gaia: read %u rows at %.0f rows/sec\n", rows, rows/PerfTimer::ns_s(ns));

    // *************************************************
    // update tests
    // *************************************************
    GaiaBase::s_gaia_cache.clear();
    printf("clear cache\n");
    PerfTimer(ns, [&]() {
        rows = build_country_map(print,&countries);
    });
    printf("build_country_map: found airports for %u countries at %.0f rows/sec\n", countries, rows/PerfTimer::ns_s(ns));

    GaiaBase::s_gaia_cache.clear();
    printf("clear cache\n");
    PerfTimer(ns, [&]() {
        rows = traverse_country_map(print,&countries);
    });
    printf("traverse_country_map: traversed airports of %u countries at %.0f rows/sec\n", countries, rows/PerfTimer::ns_s(ns));

    PerfTimer(ns, [&]() {
        rows = traverse_country_map(print,&countries);
    });
    printf("traverse_country_map (cache): traversed airports of %u countries at %.0f rows/sec\n", countries, rows/PerfTimer::ns_s(ns));

    PerfTimer(ns, [&]() {
        rows = update_links();
    });
    printf("update_links: linked %u rows at %.0f rows/sec\n", rows, rows/PerfTimer::ns_s(ns));

    if (print)
        print_airports("after update_links()", 50);

    GaiaBase::s_gaia_cache.clear();
    printf("clear cache\n");
    PerfTimer(ns, [&]() {
        rows = traverse_links_list_gaia(print);
    });
    printf("traverse_links_list_gaia:  traversed %u rows at %.0f rows/sec\n", rows, rows/PerfTimer::ns_s(ns));

    PerfTimer(ns, [&]() {
        rows = traverse_links_list_gaia(print);
    });
    printf("traverse_links_list_gaia (cache):  traversed %u rows at %.0f rows/sec\n", rows, rows/PerfTimer::ns_s(ns));

    PerfTimer(ns, [&]() {
        rows = update_list_gaia();
    });
    printf("update_list_gaia:  updated %u rows at %.0f rows/sec\n", rows, rows/PerfTimer::ns_s(ns));

    PerfTimer(ns, [&]() {
        rows = clear_links();
    });
    printf("clear_links:  updated %u rows at %.0f rows/sec\n", rows, rows/PerfTimer::ns_s(ns));    
}
