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
StringRef GaiaCatalog::getClosestTable(const llvm::StringMap<int>& tableDistance)
{
    int minDistance = INT_MAX;
    StringRef closestTable;
    for (const auto& distanceIterator : tableDistance)
    {
        if (distanceIterator.second < minDistance)
        {
            minDistance = distanceIterator.second;
            closestTable = distanceIterator.first();
        }
    }

    return closestTable;
}

// Find shortest navigation path between 2 tables. If no path between tables exist or multiple shortest paths exist, return false.
// The shortest path is returned through path parameter passed by reference
// If reportError is true, report errors, otherwise silently return false.
bool GaiaCatalog::findNavigationPath(StringRef src, StringRef dst, SmallVector<string, 8>& path, bool reportErrors)
{
    if (src.empty() || dst.empty())
    {
        return false;
    }

    if (src == dst)
    {
        return true;
    }
    const auto& tableData = getCatalogTableData();

    IntrusiveRefCntPtr<DiagnosticOptions> diagOpts = new DiagnosticOptions();
    // diagClient is passed to diags object below that takes full ownership.
    TextDiagnosticPrinter *diagClient = new TextDiagnosticPrinter(llvm::errs(), &*diagOpts);
    IntrusiveRefCntPtr<DiagnosticIDs> diagID(new DiagnosticIDs());
    DiagnosticsEngine diags(diagID, &*diagOpts, diagClient);

    if (!findNavigationPath(src, dst, path, tableData))
    {
        if (reportErrors)
        {
            diags.Report(diag::err_no_path) << src << dst;
        }
        return false;
    }

    const size_t pathLength = path.size();
    // Remove edges from the original shortest path and check if a shortest path with the same length can be found.
    for (size_t pathIndex = 0; pathIndex < pathIndex - 1; ++pathIndex)
    {
        SmallVector<string, 8> tempPath;
        llvm::StringMap<CatalogTableData> graphData(tableData);
        const auto& edgeSrc = path[pathIndex];
        const auto& edgeDst = path[pathIndex + 1];
        const auto& graphIterator = graphData.find(edgeSrc);

        for (auto linkIterator = graphIterator->second.linkData.begin(); linkIterator != graphIterator->second.linkData.end();)
        {
            if (linkIterator->second.targetTable == edgeDst)
            {
                auto toErase = linkIterator;
                graphIterator->second.linkData.erase(toErase);
                ++linkIterator;
            }
            else
            {
                ++linkIterator;
            }
        }

        if (findNavigationPath(src, dst, tempPath, graphData))
        {
            if (tempPath.size() == pathLength)
            {
                if (reportErrors)
                {
                    diags.Report(diag::err_multiple_shortest_paths) << src << dst;
                }
                return false;
            }
        }
    }

    return true;
}

// Find shortest navigation path between 2 tables.
bool GaiaCatalog::findNavigationPath(StringRef src, StringRef dst, llvm::SmallVector<string, 8>& path, const llvm::StringMap<CatalogTableData>& graphData)
{
    if (src == dst)
    {
        return true;
    }

    llvm::StringMap<int> tableDistance;
    llvm::StringMap<string> previousTable;
    llvm::StringMap<string> tableNavigation;

    for (const auto& graphDataIterator : graphData)
    {
        tableDistance[graphDataIterator.first()] = INT_MAX;
    }
    tableDistance[src] = 0;

    string closestTable;

    while (closestTable != dst)
    {
        closestTable = getClosestTable(tableDistance);
        if (closestTable.empty())
        {
            return false;
        }

        if (closestTable == dst)
        {
            break;
        }

        int distance = tableDistance[closestTable];

        tableDistance.erase(closestTable);

        auto tableIterator = graphData.find(closestTable);
        for (const auto& linkIterator : tableIterator->second.linkData)
        {
            StringRef tableName = linkIterator.second.targetTable;
            if (tableDistance.find(tableName) != tableDistance.end())
            {
                if (tableDistance[tableName] > distance + 1)
                {
                    tableDistance[tableName] = distance + 1;
                    previousTable[tableName] = closestTable;
                    tableNavigation[tableName] = linkIterator.second.targetTable;
                }
            }
        }
    }

    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    StringRef tbl = dst;
    auto previousTableIterator = previousTable.find(tbl);
    while (previousTableIterator != previousTable.end())
    {
        path.insert(path.begin(), tableNavigation[tbl]);
        tbl = previousTableIterator->second;;
        previousTableIterator = previousTable.find(tbl);
    }
    return true;
}
