//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Gaia Extensions semantic checks for Sema
// interface.
//
//===----------------------------------------------------------------------===//
/////////////////////////////////////////////
// Modifications Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "clang/Sema/GaiaCatalogFacade.hpp"

#include <exception>
#include <iostream>
#include <optional>

#include "clang/Sema/Sema.h"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/common/retail_assert.hpp"

using namespace gaia::catalog;

namespace clang
{
namespace gaia_catalog
{

std::string table_facade_t::table_name() const
{
    return m_table.name();
}

std::string table_facade_t::table_type() const
{
    return std::to_string(m_table.type());
}

std::string table_facade_t::class_name() const
{
    return std::string(m_table.name()) + "_t";
}

std::vector<field_facade_t> table_facade_t::fields() const
{
    return m_fields;
}

table_facade_t::table_facade_t(gaia::catalog::gaia_table_t table)
    : m_table(std::move(table))
{
    for (auto& field : m_table.gaia_fields())
    {
        m_fields.emplace_back(field);
    }

    for (auto& relationship : m_table.outgoing_relationships())
    {
        m_outgoing_links.emplace_back(relationship, true);
    }
}

std::vector<link_facade_t> table_facade_t::outgoing_links() const
{
    return m_outgoing_links;
}

std::string field_facade_t::field_name() const
{
    return m_field.name();
}

bool field_facade_t::is_string() const
{
    return m_field.type() == static_cast<uint8_t>(::gaia::common::data_type_t::e_string);
}

bool field_facade_t::is_vector() const
{
    return m_field.repeated_count() != 1;
}

std::string field_facade_t::table_name() const
{
    return m_field.table().name();
}

QualType field_facade_t::field_type(ASTContext& context) const
{
    // Clang complains if we add a default clause to a switch that covers all values of an enum,
    // so this code is written to avoid that.
    QualType returnType = context.VoidTy;

    switch (static_cast<::gaia::catalog::data_type_t>(m_field.type()))
    {
    case gaia::catalog::data_type_t::e_bool:
        returnType = context.BoolTy;
        break;
    case gaia::catalog::data_type_t::e_int8:
        returnType = context.SignedCharTy;
        break;
    case gaia::catalog::data_type_t::e_uint8:
        returnType = context.UnsignedCharTy;
        break;
    case gaia::catalog::data_type_t::e_int16:
        returnType = context.ShortTy;
        break;
    case gaia::catalog::data_type_t::e_uint16:
        returnType = context.UnsignedShortTy;
        break;
    case gaia::catalog::data_type_t::e_int32:
        returnType = context.IntTy;
        break;
    case gaia::catalog::data_type_t::e_uint32:
        returnType = context.UnsignedIntTy;
        break;
    case gaia::catalog::data_type_t::e_int64:
        returnType = context.LongLongTy;
        break;
    case gaia::catalog::data_type_t::e_uint64:
        returnType = context.UnsignedLongLongTy;
        break;
    case gaia::catalog::data_type_t::e_float:
        returnType = context.FloatTy;
        break;
    case gaia::catalog::data_type_t::e_double:
        returnType = context.DoubleTy;
        break;
    case gaia::catalog::data_type_t::e_string:
        returnType = context.getPointerType((context.CharTy).withConst());
        break;
    }

    // We should not be reaching this line with this value,
    // unless there is an error in code.
    assert(returnType != context.VoidTy);

    return returnType;
}

std::string link_facade_t::field_name() const
{
    if (m_is_from_parent)
    {
        return m_relationship.to_child_link_name();
    }
    return m_relationship.to_parent_link_name();
}

std::string link_facade_t::from_table() const
{
    if (m_is_from_parent)
    {
        return m_relationship.parent().name();
    }
    return m_relationship.child().name();
}

std::string link_facade_t::to_table() const
{
    if (m_is_from_parent)
    {
        return m_relationship.child().name();
    }
    return m_relationship.parent().name();
}

gaia_catalog_context_t::gaia_catalog_context_t()
{
    for (const auto& table : gaia::catalog::gaia_table_t::list())
    {
        m_tables_cache[table.name()].emplace_back(table);
    }

    for (const auto& field : gaia::catalog::gaia_field_t::list())
    {
        auto field_facade = field_facade_t{field};
        m_fields_cache[field.name()].push_back(field_facade);
        m_table_to_fields_cache[field_facade.table_name()].push_back(field_facade);
    }

    for (const auto& relationship : gaia::catalog::gaia_relationship_t::list())
    {
        auto from_parent_link = link_facade_t{relationship, true};
        auto from_child_link = link_facade_t{relationship, false};

        m_links_cache[from_parent_link.field_name()].push_back(from_parent_link);
        m_links_cache[from_child_link.field_name()].push_back(from_child_link);

        m_table_to_links_cache[from_parent_link.from_table()].push_back(from_parent_link);
        m_table_to_links_cache[from_child_link.from_table()].push_back(from_child_link);
    }
}

std::optional<table_facade_t> gaia_catalog_context_t::find_table(std::string name)
{
    auto pair = m_tables_cache.find(name);

    if (pair == m_tables_cache.end())
    {
        return std::nullopt;
    }

    std::vector<table_facade_t> tables = pair->second;

    if (tables.empty())
    {
        return std::nullopt;
    }
    else if (tables.size() > 1)
    {
        // TODO I believe the translation engine does not deal well with
        //   multiple databases.
        throw std::exception();
    }

    return tables.front();
}
table_facade_t gaia_catalog_context_t::get_table(std::string name)
{
    auto table = find_table(name);

    if (!table)
    {
        throw std::exception();
    }

    return *table;
}
std::vector<field_facade_t> gaia_catalog_context_t::find_fields(std::string name)
{
    auto pair = m_fields_cache.find(name);

    if (pair == m_fields_cache.end())
    {
        return {};
    }

    return pair->second;
}

std::vector<field_facade_t> gaia_catalog_context_t::find_fields_in_tables(const std::vector<std::string>& table_names, const std::string& field_name)
{
    std::vector<field_facade_t> result;

    for (std::string table_name : table_names)
    {
        auto fields = m_table_to_fields_cache[table_name];
        for (field_facade_t& field : fields)
        {
            if (field.field_name() == field_name)
            {
                result.push_back(field);
            }
        }
    }

    return result;
}

std::vector<link_facade_t> gaia_catalog_context_t::find_links(std::string name)
{
    auto pair = m_links_cache.find(name);

    if (pair == m_links_cache.end())
    {
        return {};
    }

    return pair->second;
}

std::vector<link_facade_t> gaia_catalog_context_t::find_links_in_tables(const std::vector<std::string>& table_names, const std::string& link_name)
{
    std::vector<link_facade_t> result;

    for (std::string table_name : table_names)
    {
        auto links = m_table_to_links_cache[table_name];
        for (link_facade_t& link : links)
        {
            if (link.field_name() == link_name)
            {
                result.push_back(link);
            }
        }
    }

    return result;
}

bool gaia_catalog_context_t::is_name_valid(std::string name)
{
    return (m_tables_cache.count(name) > 0)
        || (m_fields_cache.count(name) > 0)
        || (m_links_cache.count(name) > 0);
}

bool gaia_catalog_context_t::is_name_unique(std::string name)
{
    int hits = 0;

    auto table = find_table(name);

    if (table)
    {
        hits++;
    }

    auto fields = find_fields(name);

    if (!fields.empty())
    {
        hits += fields.size();
    }

    auto relationships = find_links(name);

    if (!relationships.empty())
    {
        hits += relationships.size();
    }

    return hits == 1;
}
} // namespace gaia_catalog
} // namespace clang
