/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "clang/Basic/DiagnosticSema.h"
#include "clang/Catalog/GaiaCatalog.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "llvm/Support/raw_ostream.h"

using namespace std;
using namespace clang;
using namespace clang::gaia::catalog;
using namespace ::gaia::catalog;


bool GaiaCatalog::isInitialized = false;
unordered_map<string, CatalogTableData> GaiaCatalog::catalogTableData;


class db_monitor_t
    {
    public:
        db_monitor_t()
        {
            ::gaia::db::begin_session();
            ::gaia::db::begin_transaction();
        }

        ~db_monitor_t()
        {
            ::gaia::db::commit_transaction();
            ::gaia::db::end_session();
        }
    };


// Fill internal data from catalog.
void GaiaCatalog::fillTableData()
{
    IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions();
    TextDiagnosticPrinter *DiagClient = new TextDiagnosticPrinter(llvm::errs(), &*DiagOpts);

    IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());
    DiagnosticsEngine Diags(DiagID, &*DiagOpts, DiagClient);
    try
    {
        db_monitor_t monitor;
        for (const auto& field : gaia_field_t::list())
        {
            gaia_table_t table = field.table();
            if (!table)
            {
                // TODO: Add better message. How does this happen?
                Diags.Report(diag::err_incorrect_table) << field.name();
                catalogTableData.clear();
                return;
            }

            if (table.is_system())
            {
                continue;
            }

            CatalogTableData table_data = catalogTableData[table.name()];
            if (table_data.fieldData.find(field.name()) != table_data.fieldData.end())
            {
               Diags.Report(diag::err_duplicate_field) << field.name();
                catalogTableData.clear();
                return;
            }
            CatalogFieldData field_data;
            field_data.isActive = field.active();
            field_data.position = field.position();
            field_data.isDeprecated = field.deprecated();
            field_data.fieldType = static_cast<data_type_t>(field.type());
            table_data.dbName = table.database().name();
            table_data.fieldData[field.name()] = field_data;
            catalogTableData[table.name()] = table_data;
        }

        for (const auto& relationship : gaia_relationship_t::list())
        {
            gaia_table_t child_table = relationship.child();
            if (!child_table)
            {
                // TODO:  what is the action a user can take?
                Diags.Report(diag::err_invalid_child_table) << relationship.name();
                catalogTableData.clear();
                return;
            }

            if (child_table.is_system())
            {
                continue;
            }

            gaia_table_t parent_table = relationship.parent();
            if (!parent_table)
            {
                // TODO:  what is the action a user can take?
                Diags.Report(diag::err_invalid_parent_table) << relationship.name();
                catalogTableData.clear();
                return;
            }

            if (parent_table.is_system())
            {
                continue;
            }

            CatalogLinkData to_child_link =
            {
                child_table.name(),
                static_cast<relationship_cardinality_t>(relationship.cardinality()),
                true
            };

            CatalogLinkData to_parent_link =
            {
                parent_table.name(),
                relationship_cardinality_t::one,
                false
            };

            catalogTableData[parent_table.name()].linkData[relationship.to_child_link_name()] = to_child_link;
            catalogTableData[child_table.name()].linkData[relationship.to_parent_link_name()] = to_parent_link;
        }
    }
    catch (const exception& e)
    {
        Diags.Report(diag::err_catalog_exception) << e.what();
        catalogTableData.clear();
        return;
    }
}

void GaiaCatalog::ensureInitialization()
{
    if (!isInitialized)
    {
        fillTableData();
        isInitialized = true;
    }
}
