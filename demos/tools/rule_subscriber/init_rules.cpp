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

static void rule_subscriber_commit_tx_hook()
{
    gaia_base_t::commit_hook();
    gaia::rules::log_database_event(nullptr, gaia::rules::event_type_t::transaction_commit,
        gaia::rules::event_mode_t::immediate);
}
static void rule_subscriber_begin_tx_hook()
{
    gaia_base_t::begin_hook();
    gaia::rules::log_database_event(nullptr, gaia::rules::event_type_t::transaction_begin,
        gaia::rules::event_mode_t::immediate);
}

extern "C" void initialize_rules()
{
    rule_binding_t  Table_handler("ruleset_1","rule-3",ruleset_1::Table_handler);
    rule_binding_t  TransactionRule_handler("ruleset_1","rule-4",ruleset_1::TransactionRule_handler);
    rule_binding_t  Field_handler("ruleset_1","rule-2",ruleset_1::Field_handler);
    rule_binding_t  ObjectRule_handler("ruleset_1","rule-1",ruleset_1::ObjectRule_handler);

    subscribe_database_rule(AddrBook::Employee::s_gaia_type, event_type_t::row_update, ObjectRule_handler);

    subscribe_database_rule(AddrBook::Employee::s_gaia_type, event_type_t::row_insert, ObjectRule_handler);

    field_list_t fields_1;
    fields_1.insert("last_name");
    subscribe_field_rule(AddrBook::Employee::s_gaia_type, event_type_t::field_write, fields_1, ObjectRule_handler);

    field_list_t fields_2;
    fields_2.insert("first_name");
    subscribe_field_rule(AddrBook::Employee::s_gaia_type, event_type_t::field_read, fields_2, ObjectRule_handler);

    field_list_t fields_3;
    fields_3.insert("first_name");
    subscribe_field_rule(AddrBook::Employee::s_gaia_type, event_type_t::field_write, fields_3, ObjectRule_handler);

    subscribe_database_rule(0, event_type_t::transaction_commit, ObjectRule_handler);

    field_list_t fields_4;
    fields_4.insert("last_name");
    subscribe_field_rule(AddrBook::Employee::s_gaia_type, event_type_t::field_write, fields_4, Field_handler);

    field_list_t fields_5;
    fields_5.insert("first_name");
    subscribe_field_rule(AddrBook::Employee::s_gaia_type, event_type_t::field_write, fields_5, Field_handler);

    field_list_t fields_6;
    fields_6.insert("phone_number");
    subscribe_field_rule(AddrBook::Employee::s_gaia_type, event_type_t::field_read, fields_6, Field_handler);

    subscribe_database_rule(AddrBook::Employee::s_gaia_type, event_type_t::row_delete, Table_handler);

    subscribe_database_rule(0, event_type_t::transaction_begin, TransactionRule_handler);

    subscribe_database_rule(0, event_type_t::transaction_commit, TransactionRule_handler);

    gaia::db::s_tx_begin_hook = rule_subscriber_begin_tx_hook;
    gaia::db::s_tx_commit_hook = rule_subscriber_commit_tx_hook;
}
