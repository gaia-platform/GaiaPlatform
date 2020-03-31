
#include <random>
#include "NoIUDEventsGaiaTests/tests/addr_book_gaia_generated.h"
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

int64_t create_employee(int64_t Gaia_Mgr_id_val, int64_t Gaia_FirstAddr_id_val, int64_t Gaia_FirstPhone_id_val, 
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


void GaiaNoIUDEventsTest()
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
        
    int64_t empl_node_id = create_employee(manager_id
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
    TEST_EQ(g_event_type,gaia::rules::event_type_t::col_change);
    TEST_EQ(g_event_mode,gaia::rules::event_mode_t::immediate);
    TEST_EQ(g_gaia_type, AddrBook::kEmployeeType);
    TEST_EQ(g_table_context, pEmployee);
    
    pEmployee->update_row();
    TEST_EQ(g_event_type,gaia::rules::event_type_t::col_change);
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

    gaia_mem_base::init(true);
    GaiaNoIUDEventsTest();

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

