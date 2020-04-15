# demos/rules/RuleSubscriber
This is a folder for a tool to generate a rule subscription code out of annotated cpp code. This is a temporary solution until rule generator is ready.

# Build
To build the tool, libclang-8-dev package should be installed. This package will bring internal llvm headers and libraries that are required to build the tool. If the package is not installed the build would skip building the tool.
to build follow regular pattern:
```
cd GaiaPlatform/demos
mkdir build
cd build
cmake ..
make
```
The tool will be located at build/rules/RuleSubscriber folder.

# Use 
The build accept as a parameter a name of a source file that contains annotated functions and name of an output file that will contain the rule subscription code. In addition the tool accepts as a parameter a list of folders to include files in a syntax similar to clang compiler.
The example of tool execution is:
```
~/src/GaiaPlatformRuleSubscribeTool/GaiaPlatform/demos/build$ ./rules/RuleSubscriber/rule_subscriber rules/RuleSubscriber/out.cpp -output ttt.cpp -- -I ../../production/inc/public/rules -I /usr/lib/llvm-8/lib/clang/8.0.1/include -I ../../production/inc/internal/common -I ../../production/inc/public/common -I ../../third_party/production/flatbuffers/include -I ../../production/db/storage_engine/mock
```

The example of input file:
```
#include "rules.hpp"
#include "addr_book_gaia_generated.h"
using namespace gaia::rules;

/** ruleset*/
namespace ruleset_1
{
/**
 rule-1: 
[AddrBook::Employee](update, insert);
[AddrBook::Employee.last_name](write);
[AddrBook::Employee.first_name](read, write);
[](commit)
*/
    void ObjectRule_handler(const rule_context_t *context)
    {
        int x =5;
        x=x*2;
    }

/**
 rule-2: [AddrBook::Employee.last_name](write); [AddrBook::Employee.first_name](write); [AddrBook::Employee.phone_number](read)
 */
    void Field_handler(const rule_context_t *context)
    {
        int x =5;
        x=x*2;
    }

/**
 rule-3: [AddrBook::Employee](delete)
 */
    void Table_handler(const rule_context_t *context)
    {
        int x =5;
        x=x*2;
    }

/**
 rule-4: [](begin, commit, rollback)
*/
    void TransactionRule_handler(const rule_context_t *context)
    {
        int x =5;
        x=x*2;
    }
}
```
The code that will be generated from that file using the commandline above is:  
```
#include "rules.hpp"
#include "addr_book_gaia_generated.h"

using namespace gaia::rules;

namespace ruleset_1{
void Table_handler(const rule_context_t* context);
}
namespace ruleset_1{
void ObjectRule_handler(const rule_context_t* context);
}
namespace ruleset_1{
void TransactionRule_handler(const rule_context_t* context);
}
namespace ruleset_1{
void Field_handler(const rule_context_t* context);
}

extern "C" void initialize_rules()
{
    rule_binding_t  Table_handler("ruleset_1","rule-3",ruleset_1::Table_handler);
    rule_binding_t  TransactionRule_handler("ruleset_1","rule-4",ruleset_1::TransactionRule_handler);
    rule_binding_t  Field_handler("ruleset_1","rule-2",ruleset_1::Field_handler);
    rule_binding_t  ObjectRule_handler("ruleset_1","rule-1",ruleset_1::ObjectRule_handler);

    subscribe_database_rule(AddrBook::Employee::gaia_type_id(), event_type_t::row_update, ObjectRule_handler);

    subscribe_database_rule(AddrBook::Employee::gaia_type_id(), event_type_t::row_insert, ObjectRule_handler);

    field_list_t fields_1;
    fields_1.insert("last_name");
    subscribe_field_rule(AddrBook::Employee::gaia_type_id(), event_type_t::field_write, fields_1, ObjectRule_handler);

    field_list_t fields_2;
    fields_2.insert("first_name");
    subscribe_field_rule(AddrBook::Employee::gaia_type_id(), event_type_t::field_read, fields_2, ObjectRule_handler);

    field_list_t fields_3;
    fields_3.insert("first_name");
    subscribe_field_rule(AddrBook::Employee::gaia_type_id(), event_type_t::field_write, fields_3, ObjectRule_handler);

    subscribe_database_rule(0, event_type_t::transaction_commit, ObjectRule_handler);

    field_list_t fields_4;
    fields_4.insert("last_name");
    subscribe_field_rule(AddrBook::Employee::gaia_type_id(), event_type_t::field_write, fields_4, Field_handler);

    field_list_t fields_5;
    fields_5.insert("first_name");
    subscribe_field_rule(AddrBook::Employee::gaia_type_id(), event_type_t::field_write, fields_5, Field_handler);

    field_list_t fields_6;
    fields_6.insert("phone_number");
    subscribe_field_rule(AddrBook::Employee::gaia_type_id(), event_type_t::field_read, fields_6, Field_handler);

    subscribe_database_rule(AddrBook::Employee::gaia_type_id(), event_type_t::row_delete, Table_handler);

    subscribe_database_rule(0, event_type_t::transaction_begin, TransactionRule_handler);

    subscribe_database_rule(0, event_type_t::transaction_commit, TransactionRule_handler);

    subscribe_database_rule(0, event_type_t::transaction_rollback, TransactionRule_handler);
}
```
