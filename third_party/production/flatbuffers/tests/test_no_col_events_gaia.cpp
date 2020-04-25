
#include <random>
#include "NoColEventsGaiaTests/tests/addr_book_gaia_generated.h"
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
gaia::common::gaia_base_t *g_table_context;
const char* g_field;

namespace gaia 
{
    namespace rules
    {
        bool log_database_event(common::gaia_base_t *row, event_type_t type, event_mode_t mode)
        {
            g_event_type = type;
            g_event_mode = mode;
            g_table_context = row;
            return true;
        }

        bool log_field_event(common::gaia_base_t* row, const char* field, event_type_t type, event_mode_t mode)
        {
            g_event_type = type;
            g_event_mode = mode;
            g_table_context = row;
            g_field = field;
            return true;
        }
    }
}

void verify_database_event(gaia::common::gaia_base_t* table_context, 
    gaia::rules::event_type_t event_type, gaia::rules::event_mode_t mode)
{
    TEST_EQ(g_event_type, event_type);
    TEST_EQ(g_event_mode, mode);
    TEST_EQ(g_table_context, table_context);
}

void verify_field_event(gaia::common::gaia_base_t* table_context, const char* field,
    gaia::rules::event_type_t event_type, gaia::rules::event_mode_t mode)
{
    verify_database_event(table_context, event_type, mode);
    TEST_EQ_STR(g_field, field);
}

void GaiaNoColEventsTest()
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

    gaia_mem_base::init("no_col_events", true);

    GaiaNoColEventsTest();

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

