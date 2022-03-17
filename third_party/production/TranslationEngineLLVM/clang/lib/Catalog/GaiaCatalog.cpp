/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "clang/Basic/DiagnosticSema.h"
#include "clang/Catalog/GaiaCatalog.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "llvm/Support/raw_ostream.h"

#include "gaia_internal/common/system_error.hpp"

using namespace std;
using namespace clang;
using namespace clang::gaia::catalog;
using namespace ::gaia::catalog;

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

const llvm::StringMap<CatalogTableData>& clang::gaia::catalog::getCatalogTableData()
{
    return GaiaCatalog::get().getCatalogTableData();
}

bool clang::gaia::catalog::findNavigationPath(llvm::StringRef src, llvm::StringRef dst, llvm::SmallVector<string, 8>& currentPath, bool reportErrors)
{
    return GaiaCatalog::get().findNavigationPath(src, dst, currentPath, reportErrors);
}

// This pointer is filled in either by gaiat invoking GaiaCatalog::create() (our main use case)
// or by get() below when we are running clang standalone with gaia extensions enabled for the
// LLVM parser tests.
//
// The create() and get() methods are not intended to be called concurrently.
// If multi-threaded use of these APIs becomes a use-case then we'll need to
// add synchronization around the initialization of this catalog instance pointer.
std::unique_ptr<GaiaCatalog> GaiaCatalog::s_catalog_ptr;


void GaiaCatalog::create(clang::DiagnosticsEngine& diag)
{
    s_catalog_ptr = std::make_unique<GaiaCatalog>(diag);
}

GaiaCatalog& GaiaCatalog::get()
{
    // If running under gaiat, s_catalog_ptr will be setup with the DiagnosticsEngine from the compiler
    // instance that gaiat creates. Otherwise lazily create a DiagnosticEngine here and wrap a
    // catalog instance around it.
    if (!s_catalog_ptr)
    {
        static std::unique_ptr<DiagnosticsEngine> s_diags_ptr;

        IntrusiveRefCntPtr<DiagnosticOptions> diagOpts = new DiagnosticOptions();
        TextDiagnosticPrinter *diagClient = new TextDiagnosticPrinter(llvm::errs(), &*diagOpts);
        IntrusiveRefCntPtr<DiagnosticIDs> diagID(new DiagnosticIDs());

        s_diags_ptr = std::make_unique<clang::DiagnosticsEngine>(diagID, &*diagOpts, diagClient);
        create(*(s_diags_ptr.get()));
    }

    return *(s_catalog_ptr.get());
}

GaiaCatalog::GaiaCatalog(DiagnosticsEngine& diags)
: m_isInitialized(false), m_diags(diags)
{
}

const llvm::StringMap<CatalogTableData>& GaiaCatalog::getCatalogTableData()
{
    ensureInitialization();
    return m_catalogTableData;
}

// Fill internal data from catalog.
void GaiaCatalog::fillTableData()
{
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
            table_data.dbName = table.database().name();
            m_catalogTableData[table.name()] = table_data;
        }

        for (const auto& field : gaia_field_t::list())
        {
            gaia_table_t table = field.table();
            if (!table)
            {
                // TODO: Add better message. How does this happen?
                m_diags.Report(diag::err_incorrect_table) << field.name();
                m_catalogTableData.clear();
                return;
            }

            if (field.repeated_count() > 1)
            {
                m_diags.Report(diag::err_incorrect_repeated_count) << field.name();
                m_catalogTableData.clear();
                return;
            }

            if (table.is_system())
            {
                continue;
            }

            auto table_data_iterator = m_catalogTableData.find(table.name());
            if (table_data_iterator == m_catalogTableData.end())
            {
                m_diags.Report(diag::err_wrong_table_field) << field.name() << table.name();
                m_catalogTableData.clear();
                return;
            }

            if (table_data_iterator->second.fieldData.find(field.name()) != table_data_iterator->second.fieldData.end())
            {
                m_diags.Report(diag::err_duplicate_field) << field.name();
                m_catalogTableData.clear();
                return;
            }
            CatalogFieldData field_data;
            field_data.isActive = field.active();
            field_data.position = field.position();
            field_data.isDeprecated = field.deprecated();
            field_data.isArray = field.repeated_count() == 0;
            field_data.fieldType = static_cast<data_type_t>(field.type());
            table_data_iterator->second.fieldData[field.name()] = field_data;
        }

        for (const auto& relationship : gaia_relationship_t::list())
        {
            gaia_table_t child_table = relationship.child();
            if (!child_table)
            {
                // TODO:  what is the action a user can take?
                m_diags.Report(diag::err_invalid_child_table) << relationship.name();
                m_catalogTableData.clear();
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
                m_diags.Report(diag::err_invalid_parent_table) << relationship.name();
                m_catalogTableData.clear();
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

            m_catalogTableData[parent_table.name()].linkData[relationship.to_child_link_name()] = to_child_link;
            m_catalogTableData[child_table.name()].linkData[relationship.to_parent_link_name()] = to_parent_link;
        }
    }
    catch (const ::gaia::common::system_error& e)
    {
        // [GAIAPLAT-1725] Mark a system error as a fatal error.  For example, if we can't connect
        // to the database, then there is no point in continuing to emit other errors that are
        // caused by this. Fatal errors suppress subsequent diagnostic messages.
        m_diags.Report(diag::err_catalog_exception_fatal) << e.what();
        m_catalogTableData.clear();
    }
    catch (const exception& e)
    {
        m_diags.Report(diag::err_catalog_exception) << e.what();
        m_catalogTableData.clear();
    }
}

void GaiaCatalog::ensureInitialization()
{
    if (!m_isInitialized)
    {
        fillTableData();
        m_isInitialized = true;
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

    if (!findNavigationPath(src, dst, path, tableData))
    {
        if (reportErrors)
        {
            m_diags.Report(diag::err_no_path) << src << dst;
        }
        return false;
    }

    const size_t pathLength = path.size();
    // Remove edges from the original shortest path and check if a shortest path with the same length can be found.
    for (size_t pathIndex = 0; pathIndex < pathLength - 1; ++pathIndex)
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
                    m_diags.Report(diag::err_multiple_shortest_paths) << src << dst;
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
