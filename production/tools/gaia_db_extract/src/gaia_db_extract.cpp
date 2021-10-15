/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_db_extract.hpp"

#include <memory>
#include <vector>

#include "gaia/db/db.hpp"

#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/common/system_table_types.hpp"
#include "gaia_internal/db/catalog_core.hpp"
#include "gaia_internal/db/db_types.hpp"

using namespace gaia::common;
using namespace gaia::catalog;
using namespace gaia::db;
using namespace std;

static string print_indent(int32_t level)
{
    stringstream dump;

    for (int i = 0; i < level; ++i)
    {
        dump << "    ";
    }

    return dump.str();
}

void print_field(int32_t level, gaia::catalog::gaia_field_t field)
{
    print_indent(level);
    printf("field name:  %s, id: %08lx, type: %08x\n", field.name(), uint64_t(field.gaia_id()), uint32_t(field.type()));
}

void print_table(int32_t level, gaia::catalog::gaia_table_t table)
{
    print_indent(level);
    printf("table name:  %s, id: %08lx, type: %08x\n", table.name(), uint64_t(table.gaia_id()), uint32_t(table.type()));
    for (auto field : table.gaia_fields())
    {
        print_field(level + 1, field);
    }
}

string print_database(int32_t level, gaia::catalog::gaia_database_t db)
{
    stringstream dump;

    dump << print_indent(level);
    dump << "\"database\": \"" << db.name() << "\",\n";
    dump << print_indent(level);
    dump << "\"tables\":\n";
    dump << print_indent(level);
    dump << "\n";

    for (auto table : db.gaia_tables())
    {
        print_table(level + 1, table);
    }

    return dump.str();
}

stringstream gaia_db_extract(gaia_id_t, gaia_id_t, bool, bool, bool, int, type_vector)
{
    stringstream dump;
    bool first_loop = true;

    begin_transaction();
    dump << "{\n    \"databases\":\n    [\n";
    for (auto db : gaia::catalog::gaia_database_t::list())
    {
        if (strlen(db.name()) == 0 || !strcmp(db.name(), "catalog") || !strcmp(db.name(), "()"))
        {
            continue;
        }
        dump << print_database(2, db);
    }
    dump << "    ]\n}\n";

    commit_transaction();

    return dump;
}
