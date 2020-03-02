/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cstring>
#include <cstdint>
#include <list>
#include <map>
#include <fstream>
#include <uuid/uuid.h>
#include "addr_book_generated.h" // include both flatbuffer types and object API for testing 
#include "GaiaObj.h"
#include "NullableString.h"
#include "PerfTimer.h"
#include "CSVRow.h"

using namespace std;
using namespace gaia_se;
using namespace AddrBook;

bool print;
map<string, gaia_id_t> state_map;

namespace AddrBook {
    static const gaia_se::gaia_type_t kEmployeeType = 4;
    static const gaia_se::gaia_type_t kPhoneType = 5;
    static const gaia_se::gaia_type_t kAddressType = 6;
};

struct Employee : public GaiaObj<AddrBook::kEmployeeType, Employee, employee, employeeT>
{
    int64_t Gaia_id() const { return get(Gaia_id); }
    int64_t Gaia_Mgr_id() const { return get(Gaia_Mgr_id); }
    int64_t Gaia_FirstAddr_id() const { return get(Gaia_FirstAddr_id); }
    int64_t Gaia_FirstPhone_id() const { return get(Gaia_FirstPhone_id); }
    int64_t Gaia_FirstProvision_id() const { return get(Gaia_FirstProvision_id); }
    int64_t Gaia_FirstSalary_id() const { return get(Gaia_FirstSalary_id); }
    const char * name_first() const { return get_str(name_first); }
    const char * name_last() const { return get_str(name_last); }
    const char * ssn() const { return get_str(ssn); }
    int64_t hire_date() const { return get(hire_date); }
    const char *  email() const { return get_str(email); }
    const char *  web() const { return get_str(web); }

    void set_Gaia_id(uint64_t i) { set(Gaia_id, i); }
    void set_Gaia_Mgr_id(uint64_t i) { set(Gaia_Mgr_id, i); }
    void set_Gaia_FirstAddr_id(uint64_t i) { set(Gaia_FirstAddr_id, i); }
    void set_Gaia_FirstPhone_id(uint64_t i) { set(Gaia_FirstPhone_id, i); }
    void set_Gaia_FirstProvision_id(uint64_t i) { set(Gaia_FirstProvision_id, i); }
    void set_Gaia_FirstSalary_id(uint64_t i) { set(Gaia_FirstSalary_id, i); }
    void set_name_first(const char * s) { set(name_first, s); }
    void set_name_last(const char * s) { set(name_last, s); }
    void set_ssn(const char * s) { set(ssn, s); }
    void set_hire_date(uint64_t i) { set(hire_date, i); }
    void set_email(const char * s) { set(email, s); }
    void set_web(const char * s) { set(web, s); }
}; // Employee 

struct Phone : public GaiaObj<AddrBook::kPhoneType, Phone, phone, phoneT>
{
    int64_t Gaia_id() const { return get(Gaia_id); }
    int64_t Gaia_NextPhone_id() const { return get(Gaia_NextPhone_id); }
    const char * phone_number() const { return get_str(phone_number); }
    const char * type() const { return get_str(type); }
    int32_t primary() const { return get(primary); }

    void set_Gaia_id(uint64_t i) { set(Gaia_id, i); }
    void set_NextPhone_id(uint64_t i) { set(Gaia_NextPhone_id, i); }
    void set_phone_number(const char * s) { set(phone_number, s); }
    void set_type(const char * s) { set(type, s); }
    void set_primary(uint32_t i) { set(primary, i); }
}; // Phone

struct Address : public GaiaObj<AddrBook::kAddressType, Address, address, addressT>
{
    int64_t Gaia_id() const { return get(Gaia_id); }
    int64_t Gaia_NextAddr_id() const { return get(Gaia_NextAddr_id); }
    int64_t Gaia_NextState_id() const { return get(Gaia_NextState_id); }
    const char * street() const { return get_str(street); }
    const char * apt_suite() const { return get_str(apt_suite); }
    const char * city() const { return get_str(city); }
    const char * state() const { return get_str(state); }
    const char * postal() const { return get_str(postal); }
    const char * country() const { return get_str(country); }
    int32_t current() const { return get(current); }

    void set_Gaia_id(uint64_t i) { set(Gaia_id, i); }
    void set_Gaia_NextAddr_id(uint64_t i) { set(Gaia_NextAddr_id, i); }
    void set_Gaia_NextState_id(uint64_t i) { set(Gaia_NextState_id, i); }
    void set_street(const char * s) { set(street, s); }
    void set_apt_suite(const char * s) { set(apt_suite, s); }
    void set_city(const char * s) { set(city, s); }
    void set_state(const char * s) { set(state, s); }
    void set_postal(const char * s) { set(postal, s); }
    void set_country(const char * s) { set(country, s); }
    void set_current(uint32_t i) { set(current, i); }
}; // Address

class GaiaTx
{
public:
    GaiaTx(std::function<bool ()> fn) {
        gaia_se::begin_transaction();
        if (fn())
            gaia_se::commit_transaction();
        else
            gaia_se::rollback_transaction();
    }
};

uint32_t traverse_employees()
{
    uint32_t i = 0;
    GaiaTx([&]() {
        Employee * ep;
        for(ep = Employee::GetFirst();
            ep;
            ep = ep->GetNext())
        {
            if (print)
                printf("%s, %s %s, %s\n", ep->name_first(), ep->name_last(), ep->email(), ep->web());
            Phone * pp;
            for (pp = Phone::GetRowById(ep->Gaia_FirstPhone_id());
                 pp;
                 pp = Phone::GetRowById(pp->Gaia_NextPhone_id()))
            {
                if (print)
                    printf("   %s (%s)\n", pp->phone_number(), pp->type());
            }
            Address * ap;
            for (ap = Address::GetRowById(ep->Gaia_FirstAddr_id());
                 ap;
                 ap = Address::GetRowById(ap->Gaia_NextAddr_id()))
            {
                if (print)
                    printf("   %s\n   %s, %s  %s\n   %s\n", ap->street(), ap->city(), ap->state(),
                        ap->postal(), ap->country());
            }
            i++;
        }
        return true;
    });
    return i;
}

gaia_id_t get_next_id()
{
    // return ++_node_id;
    uuid_t uuid;
    gaia_id_t _node_id;
    uuid_generate(uuid);
    memcpy(&_node_id, uuid, sizeof(gaia_id_t));
    _node_id &= ~0x8000000000000000;
    return _node_id;
}

uint32_t build_state_map(bool print, int32_t* states)
{
    uint32_t i = 0;
    *states = 0;
    GaiaTx([&]() {
        Employee * ep;
        for(ep = Employee::GetFirst();
            ep;
            ep = ep->GetNext())
        {
            Address * ap;
            for (ap = Address::GetRowById(ep->Gaia_FirstAddr_id());
                 ap;
                 ap = Address::GetRowById(ap->Gaia_NextAddr_id()))
            {
                auto it = state_map.find(ap->state());
                auto id = ap->Gaia_id();
                gaia_id_t head_id;
                if (it != state_map.end()) {
                    head_id = it->second;
                    // new row becomes new head
                    auto head_ap = Address::GetRowById(head_id);
                    ap->set_Gaia_NextState_id(head_id);
                    ap->Update();
                    head_ap->Update();
                }
                else {
                    if (print)
                        printf("New State: %s\n", ap->state());
                    (*states)++;
                }
                state_map[ap->state()] = id;
                i++;
            }
        }
        return true;
    });
    return i;
}

uint32_t traverse_state_map(bool print, int32_t* states)
{
    uint32_t i = 0;
    *states = 0;
    GaiaTx([&]() {
        for (map<string, gaia_id_t>::iterator it = state_map.begin();
             it != state_map.end();
             ++it)
        {
            if (print)
                printf("====State Addresses for %s====\n", it->first.c_str());
            auto head_id = it->second;
            Address * ap;
            for (ap = Address::GetRowById(head_id);
                 ap;
                 ap = Address::GetRowById(ap->Gaia_NextState_id()))
            {
                auto ap_id = ap->Gaia_id();
                ap_id++;
                i++;
                if (print)
                    printf("   ---\n   %s\n   %s, %s  %s\n   %s\n", ap->street(), ap->city(), ap->state(),
                        ap->postal(), ap->country());
            }
            (*states)++;
        }
        return true;
    });
    return i;
}

// uint32_t traverse_links_list_gaia(bool print)
// {
//     uint32_t i = 0;
//     GaiaTx([&]() {
//         Airport * ap = Airport::GetFirst();
//         for(; ap; ap = Airport::GetRowById(ap->Gaia_Next_id())) {
//             int32_t ap_id = ap->ap_id();
//             ap_id++;
//             if (ap->iata()){
//                 auto len = strlen(ap->iata());
//             }
//             if (print)
//                 printf("%016lx %016lx %016lx %2d %s\n", ap->Gaia_id(), ap->Gaia_Next_id(),
//                     ap->Gaia_Prev_id(), ap_id, ap->name());
//             i++;
//         }
//         return true;
//     });
//     return i;
// }

// uint32_t update_links()
// {
//     uint32_t i = 0;

//     GaiaTx([&]() 
//     {
//         gaia_id_t this_id;
//         gaia_id_t prev_id = 0;
//         Airport * prev_ap = nullptr;
//         Airport * ap = Airport::GetFirst();
//         for (; ap ; ap = ap->GetNext()) {
//             this_id = ap->Gaia_id();
//             ap->set_Gaia_Prev_id(prev_id);
//             ap->set_Gaia_Next_id(0);
//             ap->Update();
//             if (prev_ap != nullptr) {
//                 prev_ap->set_Gaia_Next_id(this_id);
//                 prev_ap->Update();
//             }
//             prev_ap = ap;
//             prev_id = this_id;
//             i++;
//         }
//         return true;
//     });
//     return i;
// }

// uint32_t update_list_gaia()
// {
//     uint32_t i = 0;

//     GaiaTx([&]() 
//     {
//         Airport * ap = Airport::GetFirst();
//         for (; ap ; ap = ap->GetNext()) {
//             ap->set_ap_id(ap->ap_id() + 10000);
//             ap->Update();
//             i++;
//         }
//         return true;
//     });
//     return i;
// }

// uint32_t clear_links()
// {
//     uint32_t i = 0;

//     GaiaTx([&]() 
//     {
//         Airport * ap = Airport::GetFirst();
//         for (; ap ; ap = ap->GetNext()) {
//             ap->set_Gaia_Next_id(0);
//             ap->set_Gaia_Prev_id(0);
//             ap->Update();
//             i++;
//         }
//         return true;
//     });
//     return i;
// }

// void print_airports(const char* msg, int count)
// {
//     GaiaTx([&]() {
//         printf("===%s===\n", msg);
//         Airport * ap = Airport::GetFirst();
//         for(int i=0; i<count && ap; i++, ap=ap->GetNext()) {
//             int32_t ap_id = ap->ap_id();
//             printf("%016lx %016lx %016lx %2d %s\n", ap->Gaia_id(), ap->Gaia_Next_id(),
//                 ap->Gaia_Prev_id(), ap_id, ap->name());
//         }
//         return true;
//     });
// }

istream& operator>>(istream& str, CSVRow& data)
{
    data.read_next(str);
    return str;
}

void employee_loader(CSVRow& row)
{
    flatbuffers::FlatBufferBuilder b(128);
    gaia_id_t empl_node_id = get_next_id();
    gaia_id_t ph1_node_id = get_next_id();
    gaia_id_t ph2_node_id = get_next_id();
    gaia_id_t addr_node_id = get_next_id();

    // printf("%s, %s %s, %s\n", row[2].col.c_str(), row[1].col.c_str(), row[10].col.c_str(), row[11].col.c_str());
    // printf("   %s\n   %s, %s  %s\n   %s\n", row[4].col.c_str(), row[5].col.c_str(), row[6].col.c_str(), row[7].col.c_str(), row[0].col.c_str());
    // printf("   %s (Mobile), %s (Home)\n", row[8].col.c_str(), row[9].col.c_str());

    // employee row
    b.Finish(CreateemployeeDirect(b, empl_node_id, 
        0,                                               // Gaia_Mgr_id
        addr_node_id,                                    // Gaia_FirstAddr_id
        ph1_node_id,                                     // Gaia_FirstPhone_id
        0,                                               // Gaia_FirstProvision_id
        0,                                               // Gaia_FirstSalary
        row[1].is_null ? nullptr : row[1].col.c_str(),   // name_first
        row[2].is_null ? nullptr : row[2].col.c_str(),   // name_last
        nullptr,                                         // ssn
        0,                                               // hire_date
        row[10].is_null ? nullptr : row[10].col.c_str(), // email
        row[11].is_null ? nullptr : row[11].col.c_str()  // web
    ));
    gaia_se_node::create(empl_node_id, AddrBook::kEmployeeType, b.GetSize(), b.GetBufferPointer());
    b.Clear();

    b.Finish(CreatephoneDirect(b, ph1_node_id, ph2_node_id, row[8].col.c_str(), "Mobile", true));
    gaia_se_node::create(ph1_node_id, AddrBook::kPhoneType, b.GetSize(), b.GetBufferPointer());
    b.Clear();

    b.Finish(CreatephoneDirect(b, ph2_node_id, 0, row[9].col.c_str(), "Home", false));
    gaia_se_node::create(ph2_node_id, AddrBook::kPhoneType, b.GetSize(), b.GetBufferPointer());
    b.Clear();
    
    b.Finish(CreateaddressDirect(b, addr_node_id, 0, 0,
        row[4].is_null ? nullptr : row[4].col.c_str(), // street
        nullptr,                                       // apt_suite
        row[5].is_null ? nullptr : row[5].col.c_str(), // city
        row[6].is_null ? nullptr : row[6].col.c_str(), // state
        row[7].is_null ? nullptr : row[7].col.c_str(), // postal
        row[0].is_null ? nullptr : row[0].col.c_str(), // country
        true                                           // primary
    ));
    gaia_se_node::create(addr_node_id, AddrBook::kAddressType, b.GetSize(), b.GetBufferPointer());

    b.Clear();
}

uint32_t loader(const char * fname)
{
    ifstream f(fname);
    if (f.fail()) {
        printf("Could not open '%s'\n", fname);
        return 0;
    }
    
    // load employees
    CSVRow row;
    uint32_t i = 0;
    GaiaTx([&]() {
        while (f >> row) {
            employee_loader(row);
            i++;
        }
        return true;
    });
    return i;
}
    
int main (int argc, const char ** argv)
{
    print = argc >= 3;

    if (argc < 2) {
        fprintf(stderr, "usage: addr_book <CSVFile> <print>\n");
        exit(1);
    }

    gaia_mem_base::init(true);

    int64_t ns;
    int32_t rows;

    // *************************************************
    // load database
    // *************************************************

    printf("----Load Employees----\n");
    PerfTimer(ns, [&]() {
        rows = loader(argv[1]);
    });
    printf("loader: loaded %u rows at %.0f rows/sec\n", rows, rows/PerfTimer::ns_s(ns));

    // *************************************************
    // read tests
    // *************************************************

    printf("----Traverse Employees----\n");
    PerfTimer(ns, [&]() {
        rows = traverse_employees();
    });
    printf("traverse_employees: read %u rows at %.0f rows/sec\n", rows, rows/PerfTimer::ns_s(ns));

    PerfTimer(ns, [&]() {
        rows = traverse_employees();
    });
    printf("traverse_employees: read %u rows at %.0f rows/sec\n", rows, rows/PerfTimer::ns_s(ns));

    // *************************************************
    // update tests
    // *************************************************
    GaiaBase::s_gaia_cache.clear();
    printf("clear cache\n");
    int32_t states;
    PerfTimer(ns, [&]() {
        rows = build_state_map(print,&states);
    });
    printf("build_state_map: found %u states at %.0f rows/sec\n", states, rows/PerfTimer::ns_s(ns));

    GaiaBase::s_gaia_cache.clear();
    printf("clear cache\n");
    PerfTimer(ns, [&]() {
        rows = traverse_state_map(print,&states);
    });
    printf("traverse_state_map: traversed addresses of %u states at %.0f rows/sec\n", states, rows/PerfTimer::ns_s(ns));

    PerfTimer(ns, [&]() {
        rows = traverse_state_map(print,&states);
    });
    printf("traverse_state_map: traversed addresses of %u states at %.0f rows/sec\n", states, rows/PerfTimer::ns_s(ns));

    // PerfTimer(ns, [&]() {
    //     rows = update_links();
    // });
    // printf("update_links: linked %u rows at %.0f rows/sec\n", rows, rows/PerfTimer::ns_s(ns));

    // if (print)
    //     print_airports("after update_links()", 50);

    // GaiaBase::s_gaia_cache.clear();
    // printf("clear cache\n");
    // PerfTimer(ns, [&]() {
    //     rows = traverse_links_list_gaia(print);
    // });
    // printf("traverse_links_list_gaia:  traversed %u rows at %.0f rows/sec\n", rows, rows/PerfTimer::ns_s(ns));

    // PerfTimer(ns, [&]() {
    //     rows = traverse_links_list_gaia(print);
    // });
    // printf("traverse_links_list_gaia (cache):  traversed %u rows at %.0f rows/sec\n", rows, rows/PerfTimer::ns_s(ns));

    // PerfTimer(ns, [&]() {
    //     rows = update_list_gaia();
    // });
    // printf("update_list_gaia:  updated %u rows at %.0f rows/sec\n", rows, rows/PerfTimer::ns_s(ns));

    // PerfTimer(ns, [&]() {
    //     rows = clear_links();
    // });
    // printf("clear_links:  updated %u rows at %.0f rows/sec\n", rows, rows/PerfTimer::ns_s(ns));    
}
