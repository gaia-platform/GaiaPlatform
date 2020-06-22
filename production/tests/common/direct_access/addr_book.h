/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// automatically generated by the FlatBuffers compiler, do not modify

#ifndef FLATBUFFERS_GENERATED_GAIA_ADDRBOOK_ADDRBOOK_H_
#define FLATBUFFERS_GENERATED_GAIA_ADDRBOOK_ADDRBOOK_H_

#include "gaia_object.hpp"
#include "addr_book_generated.h"
#include "gaia_iterators.hpp"

// NOTE: This 3-class mock header defines a hierarchy from Employee to Address to Phone.
//       This is for testing reasons. The original schema is Employee to Address and
//       Employee to Phone.

using namespace std;
using namespace gaia::common;

namespace AddrBook {

// in Employee
const int c_first_address = 0;
const int c_first_manages_employee = 1;
const int c_parent_manages_employee = 2;
const int c_next_manages_employee = 3;
const int c_num_employee_ptrs = 4;

// in Address
const int c_first_phone = 0;
const int c_parent_employee = 1;
const int c_next_address = 2;
const int c_num_address_ptrs = 3;

// in Phone
const int c_parent_address = 0;
const int c_next_phone = 1;
const int c_num_phone_ptrs = 2;

struct Employee;
struct Address;
struct Phone;

struct Employee : public gaia_object_t<1,Employee,employee,employeeT>{
    Employee() : gaia_object_t("Employee", c_num_employee_ptrs) {
        address_list.set_outer(this);
        manages_employee_list.set_outer(this);
    }
    const char* name_first () const {return GET_STR(name_first);}
    const char* name_first_original () const {return GET_STR_ORIGINAL(name_first);}
    void set_name_first(const char* val) {SET(name_first, val);}
    const char* name_last () const {return GET_STR(name_last);}
    const char* name_last_original () const {return GET_STR_ORIGINAL(name_last);}
    void set_name_last(const char* val) {SET(name_last, val);}
    const char* ssn () const {return GET_STR(ssn);}
    const char* ssn_original () const {return GET_STR_ORIGINAL(ssn);}
    void set_ssn(const char* val) {SET(ssn, val);}
    int64_t hire_date () const {return GET_CURRENT(hire_date);}
    int64_t hire_date_original () const {return GET_ORIGINAL(hire_date);}
    void set_hire_date(int64_t val) {SET(hire_date, val);}
    const char* email () const {return GET_STR(email);}
    const char* email_original () const {return GET_STR_ORIGINAL(email);}
    void set_email(const char* val) {SET(email, val);}
    const char* web () const {return GET_STR(web);}
    const char* web_original () const {return GET_STR_ORIGINAL(web);}
    void set_web(const char* val) {SET(web, val);}
    using gaia_object_t::insert_row;
    using gaia_object_t::update_row;
    using gaia_object_t::delete_row;
    static Employee* insert_row (const char* name_first_val,const char* name_last_val,const char* ssn_val,
            int64_t hire_date_val,const char* email_val,const char* web_val)
    {
        flatbuffers::FlatBufferBuilder b(128);
        b.Finish(CreateemployeeDirect(b, name_first_val,name_last_val,ssn_val,hire_date_val,email_val,web_val));
        return gaia_object_t::insert_row(b,c_num_employee_ptrs);
    }
    void insert_row() {
        gaia_object_t::insert_row(c_num_employee_ptrs);
    }
    Employee* manages_employee_owner() {
        Employee* pp = Employee::get_row_by_id(this->m_references[c_parent_manages_employee]);
        return pp;
    }
    static gaia_container_t<Employee>& employee_table() {
        static gaia_container_t<Employee> employee_table;
        return employee_table;
    }
    reference_chain_container_t<Employee,Address,c_parent_employee,c_first_address,c_next_address> address_list;
    reference_chain_container_t<Employee,Employee,c_parent_manages_employee,c_first_manages_employee,c_next_manages_employee> manages_employee_list;
private:
    friend struct gaia_object_t<1,Employee,employee,employeeT>;
    Employee(gaia_id_t id) : gaia_object_t(id, "Employee", c_num_employee_ptrs) {
        address_list.set_outer(this);
        manages_employee_list.set_outer(this);
    }
};

struct Address : public gaia_object_t<2,Address,address,addressT>{
    Address() : gaia_object_t("Address", c_num_address_ptrs) {phone_list.set_outer(this);}
    const char* street () const {return GET_STR(street);}
    const char* street_original () const {return GET_STR_ORIGINAL(street);}
    void set_street(const char* val) {SET(street, val);}
    const char* apt_suite () const {return GET_STR(apt_suite);}
    const char* apt_suite_original () const {return GET_STR_ORIGINAL(apt_suite);}
    void set_apt_suite(const char* val) {SET(apt_suite, val);}
    const char* city () const {return GET_STR(city);}
    const char* city_original () const {return GET_STR_ORIGINAL(city);}
    void set_city(const char* val) {SET(city, val);}
    const char* state () const {return GET_STR(state);}
    const char* state_original () const {return GET_STR_ORIGINAL(state);}
    void set_state(const char* val) {SET(state, val);}
    const char* postal () const {return GET_STR(postal);}
    const char* postal_original () const {return GET_STR_ORIGINAL(postal);}
    void set_postal(const char* val) {SET(postal, val);}
    const char* country () const {return GET_STR(country);}
    const char* country_original () const {return GET_STR_ORIGINAL(country);}
    void set_country(const char* val) {SET(country, val);}
    bool current () const {return GET_CURRENT(current);}
    bool current_original () const {return GET_ORIGINAL(current);}
    void set_current(bool val) {SET(current, val);}
    using gaia_object_t::insert_row;
    using gaia_object_t::update_row;
    using gaia_object_t::delete_row;
    static Address* insert_row (const char* street_val,const char* apt_suite_val,const char* city_val,
            const char* state_val,const char* postal_val,const char* country_val,bool current_val)
    {
        flatbuffers::FlatBufferBuilder b(128);
        b.Finish(CreateaddressDirect(b, street_val,apt_suite_val,city_val,state_val,postal_val,country_val,current_val));
        return gaia_object_t::insert_row(b,c_num_address_ptrs);
    }
    void insert_row() {
        gaia_object_t::insert_row(c_num_address_ptrs);
    }
    Employee* employee_owner() {
        Employee* pp = Employee::get_row_by_id(this->m_references[c_parent_employee]);
        return pp;
    }
    static gaia_container_t<Address>& address_table() {
        static gaia_container_t<Address> address_table;
        return address_table;
    }
    reference_chain_container_t<Address,Phone,c_parent_address,c_first_phone,c_next_phone> phone_list;
private:
    friend struct gaia_object_t<2,Address,address,addressT>;
    Address(gaia_id_t id) : gaia_object_t(id, "Address", c_num_address_ptrs) {phone_list.set_outer(this);}
};

struct Phone : public gaia_object_t<3,Phone,phone,phoneT>{
    Phone() : gaia_object_t("Phone", c_num_phone_ptrs) {}
    const char* phone_number () const {return GET_STR(phone_number);}
    const char* phone_number_original () const {return GET_STR_ORIGINAL(phone_number);}
    void set_phone_number(const char* val) {SET(phone_number, val);}
    const char* type () const {return GET_STR(type);}
    const char* type_original () const {return GET_STR_ORIGINAL(type);}
    void set_type(const char* val) {SET(type, val);}
    bool primary () const {return GET_CURRENT(primary);}
    bool primary_original () const {return GET_ORIGINAL(primary);}
    void set_primary(bool val) {SET(primary, val);}
    using gaia_object_t::insert_row;
    using gaia_object_t::update_row;
    using gaia_object_t::delete_row;
    static Phone* insert_row (const char* phone_number_val,const char* type_val,bool primary_val)
    {
        flatbuffers::FlatBufferBuilder b(128);
        b.Finish(CreatephoneDirect(b, phone_number_val,type_val,primary_val));
        return gaia_object_t::insert_row(b,c_num_phone_ptrs);
    }
    Address* address_owner() {
        Address* pp = Address::get_row_by_id(this->m_references[c_parent_address]);
        return pp;
    }
private:
    friend struct gaia_object_t<3,Phone,phone,phoneT>;
    Phone(gaia_id_t id) : gaia_object_t(id, "Phone", c_num_phone_ptrs) {}
};

}  // namespace AddrBook

#endif  // FLATBUFFERS_GENERATED_GAIA_ADDRBOOK_ADDRBOOK_H_
