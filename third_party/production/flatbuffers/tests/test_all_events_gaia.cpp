
#include <random>
#include "AllEventsGaiaTests/tests/addr_book_gaia_generated.h"
#include "test_assert.h"
#include "events.hpp"


gaia_id_t get_next_id()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    return dis(gen);
}

gaia::rules::event_type_t g_event_type;
gaia::rules::event_mode_t g_event_mode;
gaia::common::gaia_type_t g_gaia_type;
gaia::common::gaia_base_t *g_table_context;

namespace gaia 
{
    namespace rules
    {
         bool log_table_event(common::gaia_base_t *row, common::gaia_type_t gaia_type, event_type_t type, event_mode_t mode)
        {
            g_event_type = type;
            g_event_mode = mode;
            g_gaia_type = gaia_type;
            g_table_context = row;
            return true;
        }

        bool log_transaction_event(event_type_t type, event_mode_t mode)
        {
            g_event_type = type;
            g_event_mode = mode;
            return true;
        }
    }
}



void GaiaGetTest()
{
    AddrBook::Employee::begin_transaction();
    TEST_EQ(g_event_type,gaia::rules::event_type_t::transaction_begin);
    TEST_EQ(g_event_mode,gaia::rules::event_mode_t::immediate);
    int64_t manager_id = get_next_id();
    int64_t first_address_id = get_next_id();
    int64_t first_phone_id = get_next_id();
    int64_t first_provision_id = get_next_id();
    int64_t first_salary_id = get_next_id();
    int64_t hire_date = get_next_id();
        
    int64_t empl_node_id = AddrBook::Employee::insert_row(manager_id
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
    TEST_EQ(g_event_type,gaia::rules::event_type_t::transaction_commit);
    TEST_EQ(g_event_mode,gaia::rules::event_mode_t::immediate);
}

void GaiaSetTest()
{
    AddrBook::Employee::begin_transaction();

    TEST_EQ(g_event_type,gaia::rules::event_type_t::transaction_begin);
    TEST_EQ(g_event_mode,gaia::rules::event_mode_t::immediate);

    int64_t manager_id = get_next_id();
    int64_t first_address_id = get_next_id();
    int64_t first_phone_id = get_next_id();
    int64_t first_provision_id = get_next_id();
    int64_t first_salary_id = get_next_id();
    int64_t hire_date = get_next_id();
        
    int64_t empl_node_id = AddrBook::Employee::insert_row(manager_id
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

    // Verify we can use the generated optimized insert_row function that 
    // calls the correct flatbuffer Create<type> API.  For types that have
    // a string or vector field, insert_row will call Create<type>Direct.
    // For types without a string or vector field, insert_row will call
    // Create<type> since the Create<type>Direct call is not generated.
    int64_t birthdate_id = AddrBook::Birthdate::insert_row(1971, 7, 20);
    AddrBook::Birthdate * pBirthdate = AddrBook::Birthdate::get_row_by_id(birthdate_id);
    TEST_EQ(1971, pBirthdate->year());

    pEmployee->set_ssn("test");
    TEST_EQ_STR("test",pEmployee->ssn());
    TEST_EQ(g_event_type,gaia::rules::event_type_t::column_change);
    TEST_EQ(g_event_mode,gaia::rules::event_mode_t::immediate);
    TEST_EQ(g_gaia_type, AddrBook::kEmployeeType);
    TEST_EQ(g_table_context, pEmployee);
    
    AddrBook::Employee::commit_transaction();
    TEST_EQ(g_event_type,gaia::rules::event_type_t::transaction_commit);
    TEST_EQ(g_event_mode,gaia::rules::event_mode_t::immediate);
}

void GaiaUpdateTest()
{
    AddrBook::Employee::begin_transaction();

    TEST_EQ(g_event_type,gaia::rules::event_type_t::transaction_begin);
    TEST_EQ(g_event_mode,gaia::rules::event_mode_t::immediate);

    int64_t manager_id = get_next_id();
    int64_t first_address_id = get_next_id();
    int64_t first_phone_id = get_next_id();
    int64_t first_provision_id = get_next_id();
    int64_t first_salary_id = get_next_id();
    int64_t hire_date = get_next_id();
        
    int64_t empl_node_id = AddrBook::Employee::insert_row(manager_id
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
    TEST_EQ(g_event_type,gaia::rules::event_type_t::column_change);
    TEST_EQ(g_event_mode,gaia::rules::event_mode_t::immediate);
    TEST_EQ(g_gaia_type, AddrBook::kEmployeeType);
    TEST_EQ(g_table_context, pEmployee);
    
    pEmployee->update_row();
    TEST_EQ(g_event_type,gaia::rules::event_type_t::row_update);
    TEST_EQ(g_event_mode,gaia::rules::event_mode_t::immediate);
    TEST_EQ(g_gaia_type, AddrBook::kEmployeeType);
    TEST_EQ(g_table_context, pEmployee);
    AddrBook::Employee *pEmployee1 = AddrBook::Employee::get_row_by_id(empl_node_id);
    TEST_EQ_STR("test",pEmployee1->ssn());

    AddrBook::Employee::commit_transaction();

    TEST_EQ(g_event_type,gaia::rules::event_type_t::transaction_commit);
    TEST_EQ(g_event_mode,gaia::rules::event_mode_t::immediate);
}


int main(int /*argc*/, const char * /*argv*/[]) 
{
    InitTestEngine();

    std::string req_locale;
    if (flatbuffers::ReadEnvironmentVariable("FLATBUFFERS_TEST_LOCALE",
        &req_locale)) 
    {
        TEST_OUTPUT_LINE("The environment variable FLATBUFFERS_TEST_LOCALE=%s",
            req_locale.c_str());
        req_locale = flatbuffers::RemoveStringQuotes(req_locale);
        std::string the_locale;
        TEST_ASSERT_FUNC(
            flatbuffers::SetGlobalTestLocale(req_locale.c_str(), &the_locale));
        TEST_OUTPUT_LINE("The global C-locale changed: %s", the_locale.c_str());
    }

    gaia_mem_base::init("all_events", true);
    GaiaGetTest();
    GaiaSetTest();
    GaiaUpdateTest();

    if (!testing_fails) 
    {
        TEST_OUTPUT_LINE("ALL TESTS PASSED");
    } 
    else 
    {
        TEST_OUTPUT_LINE("%d FAILED TESTS", testing_fails);
    }
    return CloseTestEngine();
}

