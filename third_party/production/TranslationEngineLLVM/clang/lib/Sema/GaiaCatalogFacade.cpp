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

QualType link_facade_t::field_type(ASTContext& context) const
{
    return context.BoolTy;
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

} // namespace gaia_catalog
} // namespace clang
