/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

#include <flatbuffers/code_generators.h>

#include "gaia_internal/catalog/catalog_facade.hpp"

//
// Contains the classes that generate the header & cpp code
// for DAC.
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
 * Handles the generation of a DAC compilation unit, which is defined as
 * the header and cpp containing all the DAC code for a given database.
 */
class dac_compilation_unit_writer_t
{
public:
    explicit dac_compilation_unit_writer_t(gaia_database_t database)
        : m_database{database} {};

    std::string write_header();
    std::string write_cpp();
    std::string write_init();

private:
    flatbuffers::CodeWriter create_code_writer();

    std::string generate_copyright();
    std::string generate_open_header_guard();
    std::string generate_close_header_guard();
    std::string generate_includes();
    std::string generate_includes_cpp();
    std::string generate_open_namespace();
    std::string generate_close_namespace();
    std::string generate_constants();
    std::string generate_forward_declarations();
    std::string generate_ref_forward_declarations();
    std::string generate_hash_accessor();
    std::string generate_hash_accessor_cpp();

private:
    database_facade_t m_database;
};

/**
 * Handle the generation of a single DAC class header definition and
 * CPP implementations.
 *
 * Note: this class is meant to be used by dac_compilation_unit_writer_t
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
    std::string generate_gaia_typename_accessor();
    std::string generate_gaia_typename_accessor_cpp();
    std::string generate_list_accessor();
    std::string generate_list_accessor_cpp();
    std::string generate_fields_accessors();
    std::string generate_fields_accessors_cpp();
    std::string generate_incoming_links_accessors();
    std::string generate_incoming_links_accessors_cpp();
    std::string generate_outgoing_links_accessors();
    std::string generate_outgoing_links_accessors_cpp();
    std::string generate_expressions();
    std::string generate_private_constructor();
    std::string generate_friend_declarations();
    std::string generate_close_class_definition();
    std::string generate_expr_namespace();
    std::string generate_expr_instantiation_cpp();
    std::string generate_ref_class();
    std::string generate_ref_class_cpp();

private:
    table_facade_t m_table;
    size_t m_indent_level = 0;
};

} // namespace generate
} // namespace catalog
} // namespace gaia
