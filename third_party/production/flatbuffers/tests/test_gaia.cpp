
#include <algorithm>
#include <random>
#include <vector>

#include "GaiaTests/tests/addr_book_gaia_generated.h"
#include "test_assert.h"

using namespace gaia::db::triggers;

gaia_id_t get_next_id()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    return dis(gen);
}

trigger_event_t g_trigger_event;
std::vector<uint16_t> g_columns;

extern "C"
void initialize_rules()
{
}

// Todo(msj) Write an end to end test.
// Caller won't have visibility into event triggers.
// An end to end test could be, the trigger causes a rule to do a write, which could be verified by the calling code.
void verify_trigger_event(trigger_event_t& expected)
{
    // TEST_EQ(expected.event_type, g_trigger_event.event_type);
    // TEST_EQ(expected.gaia_type, g_trigger_event.gaia_type);
    // TEST_EQ(expected.record,g_trigger_event.record);
    // TEST_EQ(expected.count_columns, g_trigger_event.count_columns);
    // for (size_t i = 0; i < g_trigger_event.count_columns; i++)
    // {
    //     uint16_t column = expected.columns[i];
    //     TEST_ASSERT(std::find(g_columns.begin(), g_columns.end(), column) != g_columns.end());
    // }
}

void GaiaGetTest()
{
    begin_transaction();

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

    commit_transaction();
}

void GaiaSetTest()
{
    begin_transaction();

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

    commit_transaction();
}

void GaiaUpdateTest()
{
    begin_transaction();

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
    pEmployee->set_name_first("john");
    TEST_EQ_STR("john",pEmployee->name_first());
    pEmployee->update_row();

    // Verify two columns changed in update_row().
    vector<uint16_t> columns;
    columns.push_back(AddrBook::employee::VT_SSN);
    columns.push_back(AddrBook::employee::VT_NAME_FIRST);
    trigger_event_t expected_a = {event_type_t::row_update, 
        AddrBook::Employee::s_gaia_type, pEmployee->gaia_id(), columns.data(), 2};
    verify_trigger_event(expected_a);

    pEmployee->set_name_last("doe");
    TEST_EQ_STR("doe",pEmployee->name_last());
    pEmployee->set_name_first("jane");
    TEST_EQ_STR("jane",pEmployee->name_first());
    pEmployee->update_row();

    // Since the changed columns are cumulative until the commit happens, we now have three
    // columns changed overall (ssn, name_last, and name_first).
    columns.push_back(AddrBook::employee::VT_NAME_LAST);
    trigger_event_t expected_b = {event_type_t::row_update, 
        AddrBook::Employee::s_gaia_type, pEmployee->gaia_id(), columns.data(), 3};
    verify_trigger_event(expected_b);

    AddrBook::Employee *pEmployee1 = AddrBook::Employee::get_row_by_id(empl_node_id);
    TEST_EQ_STR("test",pEmployee1->ssn());
    TEST_EQ_STR("jane",pEmployee1->name_first());
    TEST_EQ_STR("doe",pEmployee1->name_last());

    commit_transaction();
}

void GaiaInsertTest()
{
    begin_transaction();

    int64_t manager_id = get_next_id();
    int64_t first_address_id = get_next_id();
    int64_t first_phone_id = get_next_id();
    int64_t first_provision_id = get_next_id();
    int64_t first_salary_id = get_next_id();
    int64_t hire_date = get_next_id();
    const char* name_first = "Jane";
    const char* name_last = "Smith";
    const char* ssn = "12-34-5678";
    const char* email = "jane.smith@janesmith.com";
    const char* web = "www.janesmith.com";

    AddrBook::Employee* e = new AddrBook::Employee();
    e->set_Gaia_Mgr_id(manager_id);
    e->set_Gaia_FirstAddr_id(first_address_id);
    e->set_Gaia_FirstPhone_id(first_phone_id);
    e->set_Gaia_FirstProvision_id(first_provision_id);
    e->set_Gaia_FirstSalary_id(first_salary_id);
    e->set_name_first(name_first);
    e->set_name_last(name_last);
    e->set_ssn(ssn);
    e->set_hire_date(hire_date);
    e->set_email(email);
    e->set_web(web);
    e->insert_row();
    gaia::db::gaia_id_t inserted_id = e->gaia_id();
    delete e;    

    trigger_event_t expected = {event_type_t::row_insert, 
        AddrBook::Employee::s_gaia_type, inserted_id, nullptr, 0};
    verify_trigger_event(expected);

    e = AddrBook::Employee::get_row_by_id(inserted_id);
    TEST_EQ(manager_id, e->Gaia_Mgr_id());
    TEST_EQ(first_address_id, e->Gaia_FirstAddr_id());
    TEST_EQ(first_phone_id, e->Gaia_FirstPhone_id());
    TEST_EQ(first_provision_id, e->Gaia_FirstProvision_id());
    TEST_EQ(first_salary_id, e->Gaia_FirstSalary_id());
    TEST_EQ(hire_date, e->hire_date());
    TEST_EQ_STR(name_first, e->name_first());
    TEST_EQ_STR(name_last, e->name_last());
    TEST_EQ_STR(ssn, e->ssn());
    TEST_EQ_STR(email, e->email());
    TEST_EQ_STR(web, e->web());
    delete e;

    commit_transaction();
}

void GaiaDeleteTest()
{
    begin_transaction();

    int64_t manager_id = get_next_id();
    int64_t first_address_id = get_next_id();
    int64_t first_phone_id = get_next_id();
    int64_t first_provision_id = get_next_id();
    int64_t first_salary_id = get_next_id();
    int64_t hire_date = get_next_id();
        
    gaia::db::gaia_id_t empl_node_id = AddrBook::Employee::insert_row(manager_id
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
    pEmployee->delete_row();

    trigger_event_t expected = {event_type_t::row_delete, 
        AddrBook::Employee::s_gaia_type, empl_node_id, nullptr, 0};
    verify_trigger_event(expected);

    commit_transaction();
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
    gaia::rules::initialize_rules_engine();
    GaiaGetTest();
    GaiaSetTest();
    GaiaUpdateTest();
    GaiaInsertTest();
    GaiaDeleteTest();

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
