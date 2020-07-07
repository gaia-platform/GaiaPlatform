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
using namespace gaia::direct_access;

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

typedef shared_ptr<Employee> Employee_ptr;
typedef unique_ptr<gaia_writer_t<1,Employee,employee,employeeT, c_num_employee_ptrs>> Employee_writer;
struct Employee : public gaia_object_t<1,Employee,employee,employeeT, c_num_employee_ptrs>{
    const char* name_first () const {return GET_STR(name_first);}
    const char* name_last () const {return GET_STR(name_last);}
    const char* ssn () const {return GET_STR(ssn);}
    int64_t hire_date () const {return GET(hire_date);}
    const char* email () const {return GET_STR(email);}
    const char* web () const {return GET_STR(web);}
    using gaia_object_t::insert_row;
    static gaia_id_t insert_row (const char* name_first_val,const char* name_last_val,const char* ssn_val,
            int64_t hire_date_val,const char* email_val,const char* web_val)
    {
        flatbuffers::FlatBufferBuilder b(128);
        b.Finish(CreateemployeeDirect(b, name_first_val,name_last_val,ssn_val,hire_date_val,email_val,web_val));
        return gaia_object_t::insert_row(b,c_num_employee_ptrs);
    }
    Employee_ptr manages_employee_owner() {
        return Employee::get_row_by_id(this->m_references[c_parent_manages_employee]);
    }
    static gaia_container_t<Employee>& employee_table() {
        static gaia_container_t<Employee> employee_table;
        return employee_table;
    }
    reference_chain_container_t<Employee,Address,c_parent_employee,c_first_address,c_next_address> address_list;
    reference_chain_container_t<Employee,Employee,c_parent_manages_employee,c_first_manages_employee,c_next_manages_employee> manages_employee_list;

    // would you generate the nested class here?
    
private:
    friend struct gaia_object_t<1,Employee,employee,employeeT, c_num_employee_ptrs>;
    Employee(gaia_id_t id) : gaia_object_t(id, "Employee")
    {
        address_list.set_outer(this);
        manages_employee_list.set_outer(this);
    }
};

typedef shared_ptr<Address> Address_ptr;
typedef unique_ptr<gaia_writer_t<2,Address,address,addressT, c_num_address_ptrs>> Address_writer;
struct Address : public gaia_object_t<2,Address,address,addressT,c_num_address_ptrs>{
    const char* street () const {return GET_STR(street);}
    const char* apt_suite () const {return GET_STR(apt_suite);}
    const char* city () const {return GET_STR(city);}
    const char* state () const {return GET_STR(state);}
    const char* postal () const {return GET_STR(postal);}
    const char* country () const {return GET_STR(country);}
    bool current () const {return GET(current);}
    using gaia_object_t::insert_row;
    static gaia_id_t insert_row (const char* street_val,const char* apt_suite_val,const char* city_val,
            const char* state_val,const char* postal_val,const char* country_val,bool current_val)
    {
        flatbuffers::FlatBufferBuilder b(128);
        b.Finish(CreateaddressDirect(b, street_val,apt_suite_val,city_val,state_val,postal_val,country_val,current_val));
        return gaia_object_t::insert_row(b,c_num_address_ptrs);
    }
    Employee_ptr employee_owner() {
        return Employee::get_row_by_id(this->m_references[c_parent_employee]);
    }
    static gaia_container_t<Address>& address_table() {
        static gaia_container_t<Address> address_table;
        return address_table;
    }
    reference_chain_container_t<Address,Phone,c_parent_address,c_first_phone,c_next_phone> phone_list;
private:
    friend struct gaia_object_t<2,Address,address,addressT, c_num_address_ptrs>;
    Address(gaia_id_t id) : gaia_object_t(id, "Address") 
    {
        phone_list.set_outer(this);
    }
};

typedef shared_ptr<Phone> Phone_ptr;
typedef unique_ptr<gaia_writer_t<3,Phone,phone,phoneT, c_num_phone_ptrs>> Phone_writer;
struct Phone : public gaia_object_t<3,Phone,phone,phoneT, c_num_phone_ptrs>{
    const char* phone_number () const {return GET_STR(phone_number);}
    const char* type () const {return GET_STR(type);}
    bool primary () const {return GET(primary);}
    using gaia_object_t::insert_row;
    static gaia_id_t insert_row (const char* phone_number_val,const char* type_val,bool primary_val)
    {
        flatbuffers::FlatBufferBuilder b(128);
        b.Finish(CreatephoneDirect(b, phone_number_val,type_val,primary_val));
        return gaia_object_t::insert_row(b,c_num_phone_ptrs);
    }
    Address_ptr address_owner() {
        return Address::get_row_by_id(this->m_references[c_parent_address]);
    }
private:
    friend struct gaia_object_t<3,Phone,phone,phoneT, c_num_phone_ptrs>;
    Phone(gaia_id_t id) : gaia_object_t(id, "Phone") {}
};

}  // namespace AddrBook

#endif  // FLATBUFFERS_GENERATED_GAIA_ADDRBOOK_ADDRBOOK_H_
