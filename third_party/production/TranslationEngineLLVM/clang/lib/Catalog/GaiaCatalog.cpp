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
llvm::StringMap<CatalogTableData> GaiaCatalog::catalogTableData;

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
        for (const auto& table : gaia_table_t::list())
        {
            if (table.is_system())
            {
                continue;
            }
            CatalogTableData table_data;
            catalogTableData[table.name()] = table_data;
        }

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

            auto table_data_iterator = catalogTableData.find(table.name());
            if (table_data_iterator == catalogTableData.end())
            {
                Diags.Report(diag::err_wrong_table_field) << field.name() << table.name();
                catalogTableData.clear();
                return;
            }
            CatalogTableData table_data = table_data_iterator->second;
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

// Auxilary function find topologically closest table.
StringRef GaiaCatalog::getClosestTable(const llvm::StringMap<int>& table_distance)
{
    int min_distance = INT_MAX;
    StringRef return_value;
    for (const auto& d : table_distance)
    {
        if (d.second < min_distance)
        {
            min_distance = d.second;
            return_value = d.first();
        }
    }

    return return_value;
}

// Find shortest navigation path between 2 tables. If multiple shortest paths exist, return an error.
bool GaiaCatalog::findNavigationPath(StringRef src, StringRef dst, SmallVector<string, 8>& current_path)
{
    if (src == dst)
    {
        return true;
    }
    const auto& table_data = getCatalogTableData();

    IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions();
    TextDiagnosticPrinter *DiagClient = new TextDiagnosticPrinter(llvm::errs(), &*DiagOpts);

    IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());
    DiagnosticsEngine Diags(DiagID, &*DiagOpts, DiagClient);

    bool return_value = findNavigationPath(src, dst, current_path, table_data);
    if (!return_value)
    {
        Diags.Report(diag::err_no_path) << src << dst;
        return false;
    }

    const size_t path_length = current_path.size();
    // Remove edges from the original shortest path and check if a shortest path with the same length can be found.
    for (size_t path_index = 0; path_index < path_length - 1; ++path_index)
    {
        SmallVector<string, 8> path;
        llvm::StringMap<CatalogTableData> graph_data(table_data);
        const auto& edge_src = current_path[path_index];
        const auto& edge_dst = current_path[path_index + 1];
        const auto& graph_itr = graph_data.find(edge_src);

        for (auto it = graph_itr->second.linkData.begin(); it != graph_itr->second.linkData.end();)
        {
            if (it->second.targetTable == edge_dst)
            {
                auto toErase = it;
                graph_itr->second.linkData.erase(toErase);
                ++it;
            }
            else
            {
                ++it;
            }
        }

        if (findNavigationPath(src, dst, path, graph_data))
        {
            if (path.size() == path_length)
            {
                Diags.Report(diag::err_multiple_shortest_paths) << src << dst;
                return false;
            }
        }
    }

    return true;
}

// Find shortest navigation path between 2 tables.
bool GaiaCatalog::findNavigationPath(StringRef src, StringRef dst, llvm::SmallVector<string, 8>& current_path, const llvm::StringMap<CatalogTableData>& graph_data)
{
    if (src == dst)
    {
        return true;
    }

    llvm::StringMap<int> table_distance;
    llvm::StringMap<string> table_prev;
    llvm::StringMap<string> table_navigation;

    for (const auto& table_description : GaiaCatalog::getCatalogTableData())
    {
        table_distance[table_description.first()] = INT_MAX;
    }
    table_distance[src] = 0;

    string closest_table;

    while (closest_table != dst)
    {
        closest_table = getClosestTable(table_distance);
        if (closest_table.empty())
        {
            return false;
        }

        if (closest_table == dst)
        {
            break;
        }

        int distance = table_distance[closest_table];

        table_distance.erase(closest_table);

        auto table_itr = graph_data.find(closest_table);
        for (const auto& it : table_itr->second.linkData)
        {
            StringRef table_name = it.second.targetTable;
            if (table_distance.find(table_name) != table_distance.end())
            {
                if (table_distance[table_name] > distance + 1)
                {
                    table_distance[table_name] = distance + 1;
                    table_prev[table_name] = closest_table;
                    table_navigation[table_name] = it.second.targetTable;
                }
            }
        }
    }

    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    StringRef tbl = dst;
    while (table_prev[tbl] != "")
    {
        current_path.insert(current_path.begin(), table_navigation[tbl]);
        tbl = table_prev[tbl];
    }
    return true;
}
