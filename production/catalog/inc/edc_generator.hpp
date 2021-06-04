/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

#include <flatbuffers/code_generators.h>

#include "catalog_facade.hpp"

//
// Contains the classes the generates the header & cpp code
// for EDC.
//
// The methods that generate cpp are suffixed with _cpp, while
// the methods that generate headers do not have any particular
// suffix.
//

namespace gaia
{
namespace catalog
{
namespace generate
{

/**
 * Handles the generation of an EDC compilation unit, which is defined as
 * the header and cpp containing all the EDC code for a given database.
 */
class edc_compilation_unit_writer_t
{
public:
    explicit edc_compilation_unit_writer_t(gaia_database_t database)
        : m_database{database} {};

    std::string write_header();
    std::string write_cpp();

private:
    flatbuffers::CodeWriter create_code_writer();

    std::string generate_copyright();
    std::string generate_open_header_guard();
    std::string generate_close_header_guard();
    std::string generate_includes();
    std::string generate_cpp_includes();
    std::string generate_open_namespace();
    std::string generate_close_namespace();
    std::string generate_constants();
    std::string generate_forward_declarations();

private:
    database_facade_t m_database;
};

/**
 * Handle the generation of a single EDC class header definition and
 * CPP implementations.
 *
 * Note: this class is meant to be used by edc_compilation_unit_writer_t
 * and rely on the code it generates (eg. namespaces, offset constants, etc..).
 */
class class_writer_t
{
public:
    explicit class_writer_t(table_facade_t table_facade)
        : m_table(std::move(table_facade)){};

    std::string write_header();
    std::string write_cpp();

private:
    flatbuffers::CodeWriter create_code_writer();
    void increment_indent();
    void decrement_indent();
    std::string generate_method_params(std::vector<field_facade_t> fields);

    std::string generate_class_section_comment_cpp();
    std::string generate_writer();
    std::string generate_class_definition();
    std::string generate_list_types();
    std::string generate_public_constructor();
    std::string generate_insert();
    std::string generate_insert_cpp();
    std::string generate_list_accessor();
    std::string generate_list_accessor_cpp();
    std::string generate_fields_accessors();
    std::string generate_fields_accessors_cpp();
    std::string generate_incoming_relationships_accessors();
    std::string generate_incoming_relationships_accessors_cpp();
    std::string generate_outgoing_relationships_accessors();
    std::string generate_outgoing_relationships_accessors_cpp();
    std::string generate_expressions();
    std::string generate_private_constructor();
    std::string generate_friend_declarations();
    std::string generate_close_class_definition();
    std::string generate_expr_namespace();
    std::string generate_expr_instantiation_cpp();

private:
    table_facade_t m_table;
    int m_indent_level = 0;
};

} // namespace generate
} // namespace catalog
} // namespace gaia
