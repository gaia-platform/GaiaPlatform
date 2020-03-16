/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cstring>
#include <cstdint>
#include <list>
#include <map>
#include <fstream>
#include "nullable_string.hpp"
#include "addr_book_generated.h" // include both flatbuffer types and object API for testing 
#include "gaia_obj.hpp"
#include "PerfTimer.h"
#include "CSVRow.h"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;
using namespace AddrBook;

bool print;
map<std::string, gaia_id_t> state_map;

namespace AddrBook {
    static const gaia::db::gaia_type_t kEmployeeType = 4;
    static const gaia::db::gaia_type_t kPhoneType = 5;
    static const gaia::db::gaia_type_t kAddressType = 6;
};

struct Employee : public gaia_obj_t<AddrBook::kEmployeeType, Employee, employee, employeeT>
{
    gaia_id_t Gaia_Mgr_id() const { return get_current(Gaia_Mgr_id); }
    gaia_id_t Gaia_FirstAddr_id() const { return get_current(Gaia_FirstAddr_id); }
    gaia_id_t Gaia_FirstPhone_id() const { return get_current(Gaia_FirstPhone_id); }
    gaia_id_t Gaia_FirstProvision_id() const { return get_current(Gaia_FirstProvision_id); }
    gaia_id_t Gaia_FirstSalary_id() const { return get_current(Gaia_FirstSalary_id); }
    const char* name_first() const { return get_str(name_first); }
    const char* name_last() const { return get_str(name_last); }
    const char* ssn() const { return get_str(ssn); }
    gaia_id_t hire_date() const { return get_current(hire_date); }
    const char*  email() const { return get_str(email); }
    const char*  web() const { return get_str(web); }

    gaia_id_t Gaia_Mgr_id_original() const { return get_original(Gaia_Mgr_id); }
    gaia_id_t Gaia_FirstAddr_id_original() const { return get_original(Gaia_FirstAddr_id); }
    gaia_id_t Gaia_FirstPhone_id_original() const { return get_original(Gaia_FirstPhone_id); }
    gaia_id_t Gaia_FirstProvision_id_original() const { return get_original(Gaia_FirstProvision_id); }
    gaia_id_t Gaia_FirstSalary_id_original() const { return get_original(Gaia_FirstSalary_id); }
    const char* name_first_original() const { return get_str_original(name_first); }
    const char* name_last_original() const { return get_str_original(name_last); }
    const char* ssn_original() const { return get_str_original(ssn); }
    gaia_id_t hire_date_original() const { return get_original(hire_date); }
    const char*  email_original() const { return get_str_original(email); }
    const char*  web_original() const { return get_str_original(web); }

    void set_Gaia_Mgr_id(gaia_id_t i) { set(Gaia_Mgr_id, i); }
    void set_Gaia_FirstAddr_id(gaia_id_t i) { set(Gaia_FirstAddr_id, i); }
    void set_Gaia_FirstPhone_id(gaia_id_t i) { set(Gaia_FirstPhone_id, i); }
    void set_Gaia_FirstProvision_id(gaia_id_t i) { set(Gaia_FirstProvision_id, i); }
    void set_Gaia_FirstSalary_id(gaia_id_t i) { set(Gaia_FirstSalary_id, i); }
    void set_name_first(const char* s) { set(name_first, s); }
    void set_name_last(const char* s) { set(name_last, s); }
    void set_ssn(const char* s) { set(ssn, s); }
    void set_hire_date(gaia_id_t i) { set(hire_date, i); }
    void set_email(const char* s) { set(email, s); }
    void set_web(const char* s) { set(web, s); }
}; // Employee 

struct Phone : public gaia_obj_t<AddrBook::kPhoneType, Phone, phone, phoneT>
{
    gaia_id_t Gaia_NextPhone_id() const { return get_current(Gaia_NextPhone_id); }
    const char* phone_number() const { return get_str(phone_number); }
    const char* type() const { return get_str(type); }
    int32_t primary() const { return get_current(primary); }

    gaia_id_t Gaia_NextPhone_id_original() const { return get_original(Gaia_NextPhone_id); }
    const char* phone_number_original() const { return get_str_original(phone_number); }
    const char* type_original() const { return get_str_original(type); }
    int32_t primary_original() const { return get_original(primary); }

    void set_NextPhone_id(gaia_id_t i) { set(Gaia_NextPhone_id, i); }
    void set_phone_number(const char* s) { set(phone_number, s); }
    void set_type(const char* s) { set(type, s); }
    void set_primary(uint32_t i) { set(primary, i); }
}; // Phone

struct Address : public gaia_obj_t<AddrBook::kAddressType, Address, address, addressT>
{
    gaia_id_t Gaia_NextAddr_id() const { return get_current(Gaia_NextAddr_id); }
    gaia_id_t Gaia_NextState_id() const { return get_current(Gaia_NextState_id); }
    const char* street() const { return get_str(street); }
    const char* apt_suite() const { return get_str(apt_suite); }
    const char* city() const { return get_str(city); }
    const char* state() const { return get_str(state); }
    const char* postal() const { return get_str(postal); }
    const char* country() const { return get_str(country); }
    int32_t current() const { return get_current(current); }

    gaia_id_t Gaia_NextAddr_id_original() const { return get_original(Gaia_NextAddr_id); }
    gaia_id_t Gaia_NextState_id_original() const { return get_original(Gaia_NextState_id); }
    const char* street_original() const { return get_str_original(street); }
    const char* apt_suite_original() const { return get_str_original(apt_suite); }
    const char* city_original() const { return get_str_original(city); }
    const char* state_original() const { return get_str_original(state); }
    const char* postal_original() const { return get_str_original(postal); }
    const char* country_original() const { return get_str_original(country); }
    int32_t current_original() const { return get_original(current); }

    void set_Gaia_NextAddr_id(gaia_id_t i) { set(Gaia_NextAddr_id, i); }
    void set_Gaia_NextState_id(gaia_id_t i) { set(Gaia_NextState_id, i); }
    void set_street(const char* s) { set(street, s); }
    void set_apt_suite(const char* s) { set(apt_suite, s); }
    void set_city(const char* s) { set(city, s); }
    void set_state(const char* s) { set(state, s); }
    void set_postal(const char* s) { set(postal, s); }
    void set_country(const char* s) { set(country, s); }
    void set_current(uint32_t i) { set(current, i); }
}; // Address

uint32_t traverse_employees()
{
    uint32_t i = 0;
    Employee* ep;
    gaia_base_t::begin_transaction();
    for(ep = Employee::get_first();
        ep;
        ep = ep->get_next())
    {
        if (print)
            printf("%s, %s %s, %s\n", ep->name_first(), ep->name_last(), ep->email(), ep->web());
        Phone* pp;
        for (pp = Phone::get_row_by_id(ep->Gaia_FirstPhone_id());
             pp;
             pp = Phone::get_row_by_id(pp->Gaia_NextPhone_id()))
        {
            if (print)
                printf("   %s (%s)\n", pp->phone_number(), pp->type());
        }
        Address* ap;
        for (ap = Address::get_row_by_id(ep->Gaia_FirstAddr_id());
             ap;
             ap = Address::get_row_by_id(ap->Gaia_NextAddr_id()))
        {
            if (print)
                printf("   %s\n   %s, %s  %s\n   %s\n", ap->street(), ap->city(), ap->state(),
                    ap->postal(), ap->country());
        }
        i++;
    }
    gaia_base_t::commit_transaction();
    return i;
}

uint32_t build_state_map(bool print, int32_t* states)
{
    uint32_t i = 0;
    *states = 0;
    Employee* ep;
    gaia_base_t::begin_transaction();
    for(ep = Employee::get_first();
        ep;
        ep = ep->get_next())
    {
        Address* ap;
        for (ap = Address::get_row_by_id(ep->Gaia_FirstAddr_id());
             ap;
             ap = Address::get_row_by_id(ap->Gaia_NextAddr_id()))
        {
            auto it = state_map.find(ap->state());
            auto id = ap->gaia_id();
            gaia_id_t head_id;
            if (it != state_map.end()) {
                head_id = it->second;
                // new row becomes new head
                auto head_ap = Address::get_row_by_id(head_id);
                ap->set_Gaia_NextState_id(head_id);
                ap->update_row();
                head_ap->update_row();
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
    gaia_base_t::commit_transaction();
    return i;
}

uint32_t traverse_state_map(bool print, int32_t* states)
{
    uint32_t i = 0;
    *states = 0;
    gaia_base_t::begin_transaction();
    for (map<std::string, gaia_id_t>::iterator it = state_map.begin();
         it != state_map.end();
         ++it)
    {
        if (print)
            printf("====State Addresses for %s====\n", it->first.c_str());
        auto head_id = it->second;
        Address* ap;
        for (ap = Address::get_row_by_id(head_id);
             ap;
             ap = Address::get_row_by_id(ap->Gaia_NextState_id()))
        {
            i++;
            if (print)
                printf("   ---\n   %s\n   %s, %s  %s\n   %s\n", ap->street(), ap->city(), ap->state(),
                    ap->postal(), ap->country());
        }
        (*states)++;
    }
    gaia_base_t::commit_transaction();
    return i;
}

uint32_t delete_employees()
{
    uint32_t i = 0;
    Employee* ep;
    gaia_base_t::begin_transaction();
    for(ep = Employee::get_first();
        ep;
        ep = Employee::get_first())
    {
        ep->delete_row();
        delete ep;
        i++;
    }
    gaia_base_t::commit_transaction();
    return i;
}

uint32_t delete_addresses()
{
    uint32_t i = 0;
    Address* ap;
    gaia_base_t::begin_transaction();
    for(ap = Address::get_first();
        ap;
        ap = Address::get_first())
    {
        ap->delete_row();
        delete ap;
        i++;
    }
    gaia_base_t::commit_transaction();
    return i;
}

uint32_t delete_phones()
{
    uint32_t i = 0;
    Phone* pp;
    gaia_base_t::begin_transaction();
    for(pp = Phone::get_first();
        pp;
        pp = Phone::get_first())
    {
        pp->delete_row();
        delete pp;
        i++;
    }
    gaia_base_t::commit_transaction();
    return i;
}

istream& operator>>(istream& str, CSVRow& data)
{
    data.read_next(str);
    return str;
}

void employee_loader(CSVRow& row)
{
    // current address row
    auto a = new Address();
    if (!row[4].is_null)
        a->set_street(row[4].col.c_str());
    if (!row[5].is_null)
        a->set_city(row[5].col.c_str());
    if (!row[6].is_null)
        a->set_state(row[6].col.c_str());
    if (!row[7].is_null)
        a->set_postal(row[7].col.c_str());
    if (!row[0].is_null)
        a->set_country(row[0].col.c_str());
    a->set_current(true);
    a->insert_row();
    auto addr_node_id = a->gaia_id();

    // second phone row
    auto p = new Phone();
    p->set_phone_number(row[9].col.c_str());
    p->set_type("Home");
    p->set_primary(false);
    p->insert_row();
    auto ph2_node_id = p->gaia_id();

    // primary phone row
    p = new Phone();
    p->set_NextPhone_id(ph2_node_id);
    p->set_phone_number(row[8].col.c_str());
    p->set_type("Mobile");
    p->set_primary(true);
    p->insert_row();
    auto ph1_node_id = p->gaia_id();

    // employee row
    auto e = new Employee();
    e->set_Gaia_FirstAddr_id(addr_node_id);
    e->set_Gaia_FirstPhone_id(ph1_node_id);
    if (!row[1].is_null)
        e->set_name_first(row[1].col.c_str());
    if (!row[2].is_null)
        e->set_name_last(row[2].col.c_str());
    if (!row[10].is_null)
        e->set_email(row[10].col.c_str());
    if (!row[11].is_null)
        e->set_web(row[11].col.c_str());
    e->insert_row();
}

uint32_t loader(const char* fname)
{
    ifstream f(fname);
    if (f.fail()) {
        printf("Could not open '%s'\n", fname);
        return 0;
    }
    
    // load employees
    CSVRow row;
    uint32_t i = 0;
    gaia_base_t::begin_transaction();
    while (f >> row) {
        employee_loader(row);
        i++;
    }
    gaia_base_t::commit_transaction();
    return i;
}
    
int main (int argc, const char ** argv)
{
    print = argc >= 3;

    if (argc < 2) {
        printf("usage: addr_book <CSVFile> <print>\n");
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
    int32_t states;
    PerfTimer(ns, [&]() {
        rows = build_state_map(print,&states);
    });
    printf("build_state_map: found %u states at %.0f rows/sec\n", states, rows/PerfTimer::ns_s(ns));

    PerfTimer(ns, [&]() {
        rows = traverse_state_map(print,&states);
    });
    printf("traverse_state_map: traversed addresses of %u states at %.0f rows/sec\n", states, rows/PerfTimer::ns_s(ns));

    PerfTimer(ns, [&]() {
        rows = traverse_state_map(print,&states);
    });
    printf("traverse_state_map: traversed addresses of %u states at %.0f rows/sec\n", states, rows/PerfTimer::ns_s(ns));

    PerfTimer(ns, [&]() {
        rows = delete_employees();
    });
    printf("delete_employees: deleted %u employees at %.0f rows/sec\n", rows, rows/PerfTimer::ns_s(ns));

    PerfTimer(ns, [&]() {
        rows = delete_phones();
    });
    printf("delete_employees: deleted %u phones at %.0f rows/sec\n", rows, rows/PerfTimer::ns_s(ns));

    PerfTimer(ns, [&]() {
        rows = delete_addresses();
    });
    printf("delete_addresses: deleted %u addresses at %.0f rows/sec\n", rows, rows/PerfTimer::ns_s(ns));

    printf("----Traverse Employees----\n");
    PerfTimer(ns, [&]() {
        rows = traverse_employees();
    });
    printf("traverse_employees: read %u rows at %.0f rows/sec\n", rows, rows/PerfTimer::ns_s(ns));
}
