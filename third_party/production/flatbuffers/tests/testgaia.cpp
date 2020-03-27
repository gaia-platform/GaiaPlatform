
#include <random>
#include "addr_book_gaia_generated.h"
#include "test_assert.h"
#include "events.hpp"


gaia_id_t get_next_id()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    return dis(gen);
}

gaia::events::event_type eventType;
gaia::events::event_mode eventMode;

namespace gaia 
{
    namespace events
    {
        bool log_table_event(common::gaia_base_t *row, event_type type, event_mode mode)
        {
            eventType = type;
            eventMode = mode;
            return true;
        }

        bool log_transaction_event(event_type type, event_mode mode)
        {
            return true;
        }
    }
}

int64_t create_emloyee(int64_t Gaia_Mgr_id_val, int64_t Gaia_FirstAddr_id_val, int64_t Gaia_FirstPhone_id_val, 
    int64_t Gaia_FirstProvision_id_val, int64_t Gaia_FirstSalary_id_val, const char *name_first_val, 
    const char *name_last_val, const char *ssn_val, int64_t hire_date_val, const char *email_val, 
    const char *web_val)
{
    auto employee = new AddrBook::Employee();
    employee->set_Gaia_Mgr_id(Gaia_Mgr_id_val);
    employee->set_Gaia_FirstAddr_id(Gaia_FirstAddr_id_val);
    employee->set_Gaia_FirstPhone_id(Gaia_FirstPhone_id_val);
    employee->set_Gaia_FirstProvision_id(Gaia_FirstProvision_id_val);
    employee->set_Gaia_FirstSalary_id(Gaia_FirstSalary_id_val);
    employee->set_name_first(name_first_val);
    employee->set_name_last(name_last_val);
    employee->set_ssn(ssn_val);
    employee->set_hire_date(hire_date_val);
    employee->set_email(email_val);
    employee->set_web(web_val);
    employee->insert_row();
    return employee->gaia_id();
}

void GaiaGetTest()
{
    AddrBook::Employee::begin_transaction();
    
    int64_t manager_id = get_next_id();
    int64_t first_address_id = get_next_id();
    int64_t first_phone_id = get_next_id();
    int64_t first_provision_id = get_next_id();
    int64_t first_salary_id = get_next_id();
    int64_t hire_date = get_next_id();
        
    int64_t empl_node_id = create_emloyee(manager_id
        ,first_address_id
        ,first_phone_id
        ,first_provision_id
        ,first_salary_id
        ,"testFirst"
        ,"testLast"
        ,"testSSN"
        ,hire_date
        ,"testEmail"
        ,"testWeb"
        );
        
    AddrBook::Employee *pEmployee = AddrBook::Employee::get_row_by_id(empl_node_id);
    TEST_EQ(empl_node_id,pEmployee->gaia_id());
    TEST_EQ(manager_id,pEmployee->Gaia_Mgr_id());
    TEST_EQ(first_address_id,pEmployee->Gaia_FirstAddr_id());
    TEST_EQ(first_phone_id,pEmployee->Gaia_FirstPhone_id());
    TEST_EQ(first_provision_id,pEmployee->Gaia_FirstProvision_id());
    TEST_EQ(first_salary_id,pEmployee->Gaia_FirstSalary_id());
    TEST_EQ(hire_date,pEmployee->hire_date());
    TEST_EQ_STR("testFirst",pEmployee->name_first());
    TEST_EQ_STR("testLast",pEmployee->name_last());
    TEST_EQ_STR("testSSN",pEmployee->ssn());
    TEST_EQ_STR("testEmail",pEmployee->email());
    TEST_EQ_STR("testWeb",pEmployee->web());

    AddrBook::Employee::commit_transaction();
}

void GaiaSetTest()
{
    AddrBook::Employee::begin_transaction();

    int64_t manager_id = get_next_id();
    int64_t first_address_id = get_next_id();
    int64_t first_phone_id = get_next_id();
    int64_t first_provision_id = get_next_id();
    int64_t first_salary_id = get_next_id();
    int64_t hire_date = get_next_id();
        
    int64_t empl_node_id = create_emloyee(manager_id
        ,first_address_id
        ,first_phone_id
        ,first_provision_id
        ,first_salary_id
        ,"testFirst"
        ,"testLast"
        ,"testSSN"
        ,hire_date
        ,"testEmail"
        ,"testWeb"
        );

    AddrBook::Employee *pEmployee = AddrBook::Employee::get_row_by_id(empl_node_id);

    pEmployee->set_ssn("test");
    TEST_EQ_STR("test",pEmployee->ssn());
    TEST_EQ(eventType,gaia::events::event_type::col_change);
    TEST_EQ(eventMode,gaia::events::event_mode::immediate);
    TEST_ASSERT(pEmployee->get_changed_fields().find("ssn") != pEmployee->get_changed_fields().end());

    AddrBook::Employee::commit_transaction();
}

void GaiaUpdateTest()
{
    AddrBook::Employee::begin_transaction();

    int64_t manager_id = get_next_id();
    int64_t first_address_id = get_next_id();
    int64_t first_phone_id = get_next_id();
    int64_t first_provision_id = get_next_id();
    int64_t first_salary_id = get_next_id();
    int64_t hire_date = get_next_id();
        
    int64_t empl_node_id = create_emloyee(manager_id
        ,first_address_id
        ,first_phone_id
        ,first_provision_id
        ,first_salary_id
        ,"testFirst"
        ,"testLast"
        ,"testSSN"
        ,hire_date
        ,"testEmail"
        ,"testWeb"
        );

    AddrBook::Employee *pEmployee = AddrBook::Employee::get_row_by_id(empl_node_id);

    pEmployee->set_ssn("test");
    TEST_EQ_STR("test",pEmployee->ssn());
    TEST_EQ(eventType,gaia::events::event_type::col_change);
    TEST_EQ(eventMode,gaia::events::event_mode::immediate);
    TEST_ASSERT(pEmployee->get_changed_fields().find("ssn") != pEmployee->get_changed_fields().end());
    pEmployee->update_row();
    TEST_EQ(eventType,gaia::events::event_type::row_update);
    TEST_EQ(eventMode,gaia::events::event_mode::immediate);
    AddrBook::Employee *pEmployee1 = AddrBook::Employee::get_row_by_id(empl_node_id);
    TEST_EQ_STR("test",pEmployee1->ssn());

    AddrBook::Employee::commit_transaction();
}

void GaiaFlatBufferTests()
{
    gaia_mem_base::init(true);
    GaiaGetTest();
    GaiaSetTest();
    GaiaUpdateTest();
}