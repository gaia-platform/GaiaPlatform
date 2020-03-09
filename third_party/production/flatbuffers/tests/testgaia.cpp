
#include <random>
#include "addr_book_gaia_generated.h"
#include "test_assert.h"
#include "events.hpp"


gaia_id_t get_next_id()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    return dis(gen) & ~0x8000000000000000;
}

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

gaia::events::event_type eventType;
gaia::events::event_mode eventMode;

namespace gaia 
{
  namespace events
  {
    bool log_table_event(common::gaia_base* row, event_type type, event_mode mode)
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

void GaiaGetTest()
{
  GaiaTx([&]() {
        gaia_id_t empl_node_id = get_next_id();
        int64_t manager_id = get_next_id();
        int64_t first_address_id = get_next_id();
        int64_t first_phone_id = get_next_id();
        int64_t first_provision_id = get_next_id();
        int64_t first_salary_id = get_next_id();
        int64_t hire_date = get_next_id();
        

        AddrBook::Employee::CreateEmployee(empl_node_id
          ,empl_node_id
          ,manager_id
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

        AddrBook::Employee *pEmployee = AddrBook::Employee::GetRowById(empl_node_id);
        TEST_EQ(empl_node_id,pEmployee->Gaia_id());
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

        return true;
    });
}

void GaiaSetTest()
{
  GaiaTx([&]() {
        gaia_id_t empl_node_id = get_next_id();
        int64_t manager_id = get_next_id();
        int64_t first_address_id = get_next_id();
        int64_t first_phone_id = get_next_id();
        int64_t first_provision_id = get_next_id();
        int64_t first_salary_id = get_next_id();
        int64_t hire_date = get_next_id();
        

        AddrBook::Employee::CreateEmployee(empl_node_id
          ,empl_node_id
          ,manager_id
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

        AddrBook::Employee *pEmployee = AddrBook::Employee::GetRowById(empl_node_id);

        pEmployee->set_ssn("test");
        TEST_EQ_STR("test",pEmployee->ssn());
        TEST_EQ(eventType,gaia::events::event_type::col_change);
        TEST_EQ(eventMode,gaia::events::event_mode::immediate);
        TEST_ASSERT(pEmployee->getChangedFields().find("ssn") != pEmployee->getChangedFields().end());

        return true;
    });
}

void GaiaUpdateTest()
{
  GaiaTx([&]() {
        gaia_id_t empl_node_id = get_next_id();
        int64_t manager_id = get_next_id();
        int64_t first_address_id = get_next_id();
        int64_t first_phone_id = get_next_id();
        int64_t first_provision_id = get_next_id();
        int64_t first_salary_id = get_next_id();
        int64_t hire_date = get_next_id();
        

        AddrBook::Employee::CreateEmployee(empl_node_id
          ,empl_node_id
          ,manager_id
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

        AddrBook::Employee *pEmployee = AddrBook::Employee::GetRowById(empl_node_id);

        pEmployee->set_ssn("test");
        TEST_EQ_STR("test",pEmployee->ssn());
        TEST_EQ(eventType,gaia::events::event_type::col_change);
        TEST_EQ(eventMode,gaia::events::event_mode::immediate);
        TEST_ASSERT(pEmployee->getChangedFields().find("ssn") != pEmployee->getChangedFields().end());
        pEmployee->Update();
        TEST_EQ(eventType,gaia::events::event_type::row_update);
        TEST_EQ(eventMode,gaia::events::event_mode::immediate);
        AddrBook::Employee *pEmployee1 = AddrBook::Employee::GetRowById(empl_node_id);
        TEST_EQ_STR("test",pEmployee1->ssn());

        return true;
    });
}




void GaiaFlatBufferTests()
{
    gaia_mem_base::init(true);
    GaiaGetTest();
    GaiaSetTest();
    GaiaUpdateTest();
}