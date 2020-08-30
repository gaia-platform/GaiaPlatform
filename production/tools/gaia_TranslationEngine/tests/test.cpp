#include "gaia_barn_storage.h"
#include "rules.hpp"
using namespace gaia::rules;

int rule_called = 0;
int insert_called = 0;
int update_called = 0;

namespace test
{

 
void test_knZccFBT6FEkce4_1(const rule_context_t* context)
{
auto sensor = gaia::barn_storage::sensor_t::get(context->record);
auto incubator = sensor.i__incubator();
for (auto actuator : incubator.i__actuator_list())
{

   rule_called ++;
   
   [&]() mutable {auto w = incubator.writer(); w.max_temp= sensor.value()+4; w.update_row();return w.max_temp;}();
   if (context->last_operation(gaia::barn_storage::sensor_t::s_gaia_type) == gaia::rules::last_operation_t::row_insert)
   {
   	insert_called ++;
   }
   
   if (context->last_operation(gaia::barn_storage::sensor_t::s_gaia_type) == gaia::rules::last_operation_t::row_update)
   {
   	update_called ++;
   	if (sensor.value() > 5)
   	{
   		[&]() mutable {auto w = actuator.writer(); w.value= 1000; w.update_row();return w.value;}();
   	}
   }
  }
}
}


namespace test{
void subscribeRuleset_test()
{
rule_binding_t test_knZccFBT6FEkce4_1binding("test","test_knZccFBT6FEkce4_1",test::test_knZccFBT6FEkce4_1);
subscribe_rule(gaia::barn_storage::sensor_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields,test_knZccFBT6FEkce4_1binding);
subscribe_rule(gaia::barn_storage::sensor_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields,test_knZccFBT6FEkce4_1binding);
subscribe_rule(gaia::barn_storage::sensor_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields,test_knZccFBT6FEkce4_1binding);
}
void unsubscribeRuleset_test()
{
rule_binding_t test_knZccFBT6FEkce4_1binding("test","test_knZccFBT6FEkce4_1",test::test_knZccFBT6FEkce4_1);
unsubscribe_rule(gaia::barn_storage::sensor_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields,test_knZccFBT6FEkce4_1binding);
unsubscribe_rule(gaia::barn_storage::sensor_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields,test_knZccFBT6FEkce4_1binding);
unsubscribe_rule(gaia::barn_storage::sensor_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields,test_knZccFBT6FEkce4_1binding);
}
}
namespace gaia {
 namespace rules{
extern "C" void subscribe_ruleset(const char* ruleset_name)
{
if (strcmp(ruleset_name, "test") == 0)
{
::test::subscribeRuleset_test();
return;
}
throw ruleset_not_found(ruleset_name);
}
extern "C" void unsubscribe_ruleset(const char* ruleset_name)
{
if (strcmp(ruleset_name, "test") == 0)
{
::test::unsubscribeRuleset_test();
return;
}
throw ruleset_not_found(ruleset_name);
}
extern "C" void initialize_rules()
{
::test::subscribeRuleset_test();
}
}
}