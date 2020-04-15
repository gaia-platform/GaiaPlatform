# demos/rules/RuleSubscriber
This is a folder for a tool to generate a rule subscription code out of annotated cpp code. This is a temporary solution until rule generator is ready

# Build
To build the tool, libclang-8-dev package should be installed. This package will bring internal llvm headers and libraries that are required to build the tool. If the package is not installed the build would skip building the tool.
to build follow regular pattern:
cd GaiaPlatform/demos
mkdir build
cd build
cmake ..
make
The tool will be located at build/rules/RuleSubscriber folder

#Use 
The build accept as a parameter a name of a source file that contains annotated functions and name of an output file that will contain the rule subscription code. In addition the tool accepts as a parameter a list of folders to of include files in a syntax similar to clang compiler.
The example of tool execution is:
~/src/GaiaPlatformRuleSubscribeTool/GaiaPlatform/demos/build$ ./rules/RuleSubscriber/rule_subscriber rules/RuleSubscriber/out.cpp -output ttt.cpp -- -I ../../production/inc/public/rules -I /usr/lib/llvm-8/lib/clang/8.0.1/include -I ../../production/inc/internal/common -I ../../production/inc/public/common -I ../../third_party/production/flatbuffers/include -I ../../production/db/storage_engine/mock

The example of input file :
#include "rules.hpp"

using namespace gaia::rules;

/** ruleset*/
namespace ruleset_1
{
/**
 rule, rule_obj, TestGaia::gaia_type = 3, event_type::row_update, event_type::col_change, event_type::transaction_rollback
*/
    void ObjectRule_handler(const context_base_t *context)
    {
        int x =5;
        x=x*2;
    }
/**
 rule, rule_tr,, event_type::transaction_commit, event_type::transaction_begin
*/
    void TransactionRule_handler(const context_base_t *context)
    {
        int x =5;
        x=x*2;
    }

} 

The code that will be generated from that file using the commandline above is : 

#include "rules.hpp"
using namespace gaia::rules;
void ruleset_1::TransactionRule_handler(const context_base_t *context);
void ruleset_1::ObjectRule_handler(const context_base_t *context);
extern "C" void initialize_rules()
{
    rule_binding_t  ruleset_1::TransactionRule_handler("ruleset_1","rule_tr",ruleset_1::TransactionRule_handler);
    rule_binding_t  ruleset_1::ObjectRule_handler("ruleset_1","rule_obj",ruleset_1::ObjectRule_handler);
    subscribe_table_rule(TestGaia::gaia_type, event_type::row_update,ruleset_1::ObjectRule_handler);
    subscribe_table_rule(TestGaia::gaia_type, event_type::col_change,ruleset_1::ObjectRule_handler);
    subscribe_transaction_rule( event_type::transaction_rollback,ruleset_1::ObjectRule_handler);
    subscribe_transaction_rule( event_type::transaction_commit,ruleset_1::TransactionRule_handler);
    subscribe_transaction_rule( event_type::transaction_begin,ruleset_1::TransactionRule_handler);
} 
