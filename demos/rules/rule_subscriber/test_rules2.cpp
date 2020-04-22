#include "rules.hpp"

using namespace gaia::rules;

/** ruleset*/
namespace ruleset_1
{
/**
 rule-1: [AddrBook::Employee](update, insert);[AddrBook::Employee.last_name](write);[AddrBook::Employee.first_name](read, write);
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
