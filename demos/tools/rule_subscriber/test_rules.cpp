#include "rules.hpp"

using namespace gaia::rules;

/** ruleset*/
namespace ruleset_1
{
/**
 rule, rule_obj,TestGaia::gaia_type,  event_type::row_update, event_type::col_change, event_type::transaction_rollback
*/
    void ObjectRule_handler(const rule_context_t *context)
    {
        int x =5;
        x=x*2;
    }
/**
 rule, rule_tr,, event_type::transaction_commit, event_type::transaction_begin
*/
    void TransactionRule_handler(const rule_context_t *context)
    {
        int x =5;
        x=x*2;
    }
}