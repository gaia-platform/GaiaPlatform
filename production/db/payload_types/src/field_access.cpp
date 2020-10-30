/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "field_access.hpp"

#include <sstream>

#include "retail_assert.hpp"

using namespace std;
using namespace gaia::common;

namespace gaia
{
namespace db
{
namespace payload_types
{

invalid_schema::invalid_schema()
{
    m_message = "Invalid binary flatbuffers schema.";
}

missing_root_type::missing_root_type()
{
    m_message = "Root type is missing in binary flatbuffers schema.";
}

invalid_serialized_data::invalid_serialized_data()
{
    m_message = "Cannot deserialize data.";
}

invalid_field_position::invalid_field_position(field_position_t position)
{
    stringstream string_stream;
    string_stream << "No field could be found for position: " << position << ".";
    m_message = string_stream.str();
}

unhandled_field_type::unhandled_field_type(size_t field_type)
{
    stringstream string_stream;
    string_stream << "Cannot handle field type: " << field_type << ".";
    m_message = string_stream.str();
}

void initialize_field_cache_from_binary_schema(
    field_cache_t* field_cache,
    const uint8_t* binary_schema,
    size_t binary_schema_size)
{
    retail_assert(field_cache != nullptr, "field_cache argument should not be null.");
    retail_assert(binary_schema != nullptr, "binary_schema argument should not be null.");

    // First copy the binary schema into the field cache.
    vector<uint8_t> binary_schema_copy(binary_schema, binary_schema + binary_schema_size);
    field_cache->set_binary_schema(binary_schema_copy);

    // From now on, we work with the field cache copy of the binary schema.
    binary_schema = field_cache->get_raw_binary_schema();

    // Deserialize the schema.
    const reflection::Schema* schema = reflection::GetSchema(binary_schema);
    if (schema == nullptr)
    {
        throw invalid_schema();
    }

    // Get the type of the schema's root object.
    const reflection::Object* root_type = schema->root_table();
    if (root_type == nullptr)
    {
        throw missing_root_type();
    }

    // Get the collection of fields
    // and insert each element under its corresponding field id.
    auto fields = root_type->fields();
    for (size_t i = 0; i < fields->size(); i++)
    {
        const reflection::Field* current_field = fields->Get(i);
        field_cache->set_field(current_field->id(), current_field);
    }
}

// Data validation method.
bool verify_data_schema(
    const uint8_t* serialized_data,
    size_t serialized_data_size,
    const uint8_t* binary_schema)
{
    retail_assert(serialized_data != nullptr, "serialized_data argument should not be null.");
    retail_assert(binary_schema != nullptr, "binary_schema argument should not be null.");

    const reflection::Schema* schema = reflection::GetSchema(binary_schema);
    if (schema == nullptr)
    {
        throw invalid_schema();
    }

    // Get the type of the schema's root object.
    const reflection::Object* root_type = schema->root_table();
    if (root_type == nullptr)
    {
        throw missing_root_type();
    }

    return flatbuffers::Verify(*schema, *root_type, serialized_data, serialized_data_size);
}

// This is an internal helper for the field access methods.
// It parses the flatbuffers serialization to get the root table
// and then it retrieves the reflection::Field information
// from the type's binary schema,
// which is either loaded from the global type_cache
// or parsed from the passed-in buffer.
//
// The caller is responsible for allocating the variables
// that will hold the field_cache information.
void get_table_field_information(
    gaia_id_t type_id,
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    field_position_t field_position,
    const flatbuffers::Table*& root_table,
    auto_field_cache_t& auto_field_cache,
    field_cache_t& local_field_cache,
    const reflection::Field*& field)
{
    retail_assert(serialized_data != nullptr, "serialized_data argument should not be null.");

    // First, we parse the serialized data to get its root object.
    root_table = flatbuffers::GetAnyRoot(serialized_data);
    if (root_table == nullptr)
    {
        throw invalid_serialized_data();
    }

    // Get hold of the type cache and lookup the field cache for our type.
    type_cache_t* type_cache = type_cache_t::get();
    type_cache->get_field_cache(type_id, auto_field_cache);
    const field_cache_t* field_cache = auto_field_cache.get();

    // If data is not available for our type, we load it locally from the binary schema provided to us.
    if (field_cache == nullptr)
    {
        initialize_field_cache_from_binary_schema(&local_field_cache, binary_schema, binary_schema_size);
        field_cache = &local_field_cache;
    }

    // Lookup field information in the field cache.
    field = field_cache->get_field(field_position);
    if (field == nullptr)
    {
        throw invalid_field_position(field_position);
    }
}

// Another helper for the methods that access field arrays.
// This helper also retrieves a VectorOfAny pointer
// that allows operating on the array.
void get_table_field_array_information(
    gaia_id_t type_id,
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    field_position_t field_position,
    const flatbuffers::Table*& root_table,
    auto_field_cache_t& auto_field_cache,
    field_cache_t& local_field_cache,
    const reflection::Field*& field,
    const flatbuffers::VectorOfAny*& field_value)
{
    get_table_field_information(
        type_id, serialized_data, binary_schema, binary_schema_size, field_position,
        root_table, auto_field_cache, local_field_cache, field);

    if (field->type()->base_type() != reflection::Vector)
    {
        throw unhandled_field_type(field->type()->base_type());
    }

    field_value = GetFieldAnyV(*root_table, *field);
    if (field_value == nullptr)
    {
        throw invalid_serialized_data();
    }
}

// The access method for scalar fields and strings.
data_holder_t get_field_value(
    gaia_id_t type_id,
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    field_position_t field_position)
{
    const flatbuffers::Table* root_table = nullptr;
    auto_field_cache_t auto_field_cache;
    field_cache_t local_field_cache;
    const reflection::Field* field = nullptr;

    get_table_field_information(
        type_id, serialized_data, binary_schema, binary_schema_size, field_position,
        root_table, auto_field_cache, local_field_cache, field);

    // Read field value according to its type.
    data_holder_t result;
    result.type = field->type()->base_type();
    if (flatbuffers::IsInteger(field->type()->base_type()))
    {
        result.hold.integer_value = flatbuffers::GetAnyFieldI(*root_table, *field);
    }
    else if (flatbuffers::IsFloat(field->type()->base_type()))
    {
        result.hold.float_value = flatbuffers::GetAnyFieldF(*root_table, *field);
    }
    else if (field->type()->base_type() == reflection::String)
    {
        const flatbuffers::String* field_value = flatbuffers::GetFieldS(*root_table, *field);

        // For null strings, the field_value will come back as nullptr,
        // so just set the string_value to nullptr as well.
        result.hold.string_value = (field_value == nullptr) ? nullptr : field_value->c_str();
    }
    else
    {
        throw unhandled_field_type(field->type()->base_type());
    }

    return result;
}

// The setter method for scalar fields.
bool set_field_value(
    gaia_id_t type_id,
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    field_position_t field_position,
    const data_holder_t& value)
{
    const flatbuffers::Table* const_root_table = nullptr;
    auto_field_cache_t auto_field_cache;
    field_cache_t local_field_cache;
    const reflection::Field* field = nullptr;

    get_table_field_information(
        type_id, serialized_data, binary_schema, binary_schema_size, field_position,
        const_root_table, auto_field_cache, local_field_cache, field);

    retail_assert(
        (flatbuffers::IsInteger(field->type()->base_type()) && flatbuffers::IsInteger(value.type))
            || (flatbuffers::IsFloat(field->type()->base_type()) && flatbuffers::IsFloat(value.type)),
        "Attempt to set value of incorrect type");

    // We need to update the root_table, so we need to remove the const qualifier.
    auto root_table = const_cast<flatbuffers::Table*>(const_root_table);

    // Write field value according to its type.
    if (flatbuffers::IsInteger(field->type()->base_type()))
    {
        return flatbuffers::SetAnyFieldI(root_table, *field, value.hold.integer_value);
    }
    else if (flatbuffers::IsFloat(field->type()->base_type()))
    {
        return flatbuffers::SetAnyFieldF(root_table, *field, value.hold.float_value);
    }
    else
    {
        throw unhandled_field_type(field->type()->base_type());
    }
}

// The setter method for string fields.
vector<uint8_t> set_field_value(
    gaia_id_t type_id,
    const uint8_t* serialized_data,
    size_t serialized_data_size,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    field_position_t field_position,
    const data_holder_t& value)
{
    retail_assert(binary_schema != nullptr, "binary_schema argument should not be null.");

    const flatbuffers::Table* root_table = nullptr;
    auto_field_cache_t auto_field_cache;
    field_cache_t local_field_cache;
    const reflection::Field* field = nullptr;
    vector<uint8_t> updatable_serialized_data(serialized_data, serialized_data + serialized_data_size);

    get_table_field_information(
        type_id, updatable_serialized_data.data(), binary_schema, binary_schema_size, field_position,
        root_table, auto_field_cache, local_field_cache, field);

    retail_assert(
        field->type()->base_type() == reflection::String && value.type == reflection::String,
        "Attempt to set value of incorrect type");

    const reflection::Schema* schema = reflection::GetSchema(binary_schema);
    if (schema == nullptr)
    {
        throw invalid_schema();
    }

    string new_field_value(value.hold.string_value);

    const flatbuffers::String* field_value = flatbuffers::GetFieldS(*root_table, *field);
    if (field_value == nullptr)
    {
        throw invalid_serialized_data();
    }

    flatbuffers::SetString(
        *schema,
        new_field_value,
        field_value,
        &updatable_serialized_data);

    return updatable_serialized_data;
}

// The access method for the size of a field of array type.
size_t get_field_array_size(
    gaia_id_t type_id,
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    field_position_t field_position)
{
    const flatbuffers::Table* root_table = nullptr;
    auto_field_cache_t auto_field_cache;
    field_cache_t local_field_cache;
    const reflection::Field* field = nullptr;
    const flatbuffers::VectorOfAny* field_value = nullptr;

    get_table_field_array_information(
        type_id, serialized_data, binary_schema, binary_schema_size, field_position,
        root_table, auto_field_cache, local_field_cache, field, field_value);

    return field_value->size();
}

// The setter method for a string element of a field of array type.
std::vector<uint8_t> set_field_array_size(
    gaia_id_t type_id,
    const uint8_t* serialized_data,
    size_t serialized_data_size,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    field_position_t field_position,
    size_t new_size)
{
    retail_assert(binary_schema != nullptr, "binary_schema argument should not be null.");

    const flatbuffers::Table* root_table = nullptr;
    auto_field_cache_t auto_field_cache;
    field_cache_t local_field_cache;
    const reflection::Field* field = nullptr;
    const flatbuffers::VectorOfAny* field_value = nullptr;
    vector<uint8_t> updatable_serialized_data(serialized_data, serialized_data + serialized_data_size);

    get_table_field_array_information(
        type_id, updatable_serialized_data.data(), binary_schema, binary_schema_size, field_position,
        root_table, auto_field_cache, local_field_cache, field, field_value);

    if (new_size == field_value->size())
    {
        // No change is needed.
        return updatable_serialized_data;
    }

    const reflection::Schema* schema = reflection::GetSchema(binary_schema);
    if (schema == nullptr)
    {
        throw invalid_schema();
    }

    size_t old_size = field_value->size();
    size_t element_size = flatbuffers::GetTypeSize(field->type()->element());

    // Note: field_value may be invalidated by the following call,
    // so it should no longer be used past this point.
    //
    // If the vector is expanded,
    // new elements will be automatically set to 0 by ResizeAnyVector.
    flatbuffers::ResizeAnyVector(
        *schema,
        new_size,
        field_value,
        old_size,
        element_size,
        &updatable_serialized_data);

    return updatable_serialized_data;
}

// The access method for an element of a field of array type.
data_holder_t get_field_array_element(
    gaia_id_t type_id,
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    field_position_t field_position,
    size_t array_index)
{
    const flatbuffers::Table* root_table = nullptr;
    auto_field_cache_t auto_field_cache;
    field_cache_t local_field_cache;
    const reflection::Field* field = nullptr;
    const flatbuffers::VectorOfAny* field_value = nullptr;

    get_table_field_array_information(
        type_id, serialized_data, binary_schema, binary_schema_size, field_position,
        root_table, auto_field_cache, local_field_cache, field, field_value);

    retail_assert(array_index < field_value->size(), "Attempt to index array is out-of-bounds.");

    // Read element value according to its type.
    data_holder_t result;
    result.type = field->type()->element();
    if (flatbuffers::IsInteger(field->type()->element()))
    {
        result.hold.integer_value = flatbuffers::GetAnyVectorElemI(
            field_value, field->type()->element(), array_index);
    }
    else if (flatbuffers::IsFloat(field->type()->element()))
    {
        result.hold.float_value = flatbuffers::GetAnyVectorElemF(
            field_value, field->type()->element(), array_index);
    }
    else if (field->type()->element() == reflection::String)
    {
        const auto field_element_value
            = flatbuffers::GetAnyVectorElemPointer<const flatbuffers::String>(field_value, array_index);

        // For null strings, the field_value will come back as nullptr,
        // so just set the string_value to nullptr as well.
        result.hold.string_value = (field_element_value == nullptr) ? nullptr : field_element_value->c_str();
    }
    else
    {
        throw unhandled_field_type(field->type()->element());
    }

    return result;
}

// The setter method for a scalar element of a field of array type.
void set_field_array_element(
    gaia_id_t type_id,
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    field_position_t field_position,
    size_t array_index,
    const data_holder_t& value)
{
    const flatbuffers::Table* const_root_table = nullptr;
    auto_field_cache_t auto_field_cache;
    field_cache_t local_field_cache;
    const reflection::Field* field = nullptr;
    const flatbuffers::VectorOfAny* const_field_value = nullptr;

    get_table_field_array_information(
        type_id, serialized_data, binary_schema, binary_schema_size, field_position,
        const_root_table, auto_field_cache, local_field_cache, field, const_field_value);

    retail_assert(array_index < const_field_value->size(), "Attempt to index array is out-of-bounds.");
    retail_assert(
        (flatbuffers::IsInteger(field->type()->element()) && flatbuffers::IsInteger(value.type))
            || (flatbuffers::IsFloat(field->type()->element()) && flatbuffers::IsFloat(value.type)),
        "Attempt to set value of incorrect type");

    // We need to update the serialization, so we need to remove the const qualifier.
    auto field_value = const_cast<flatbuffers::VectorOfAny*>(const_field_value);

    // Write field value according to its type.
    if (flatbuffers::IsInteger(field->type()->element()))
    {
        flatbuffers::SetAnyVectorElemI(
            field_value, field->type()->element(), array_index, value.hold.integer_value);
    }
    else if (flatbuffers::IsFloat(field->type()->element()))
    {
        flatbuffers::SetAnyVectorElemF(
            field_value, field->type()->element(), array_index, value.hold.float_value);
    }
    else
    {
        throw unhandled_field_type(field->type()->element());
    }
}

// The setter method for a string element of a field of array type.
std::vector<uint8_t> set_field_array_element(
    gaia_id_t type_id,
    const uint8_t* serialized_data,
    size_t serialized_data_size,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    field_position_t field_position,
    size_t array_index,
    const data_holder_t& value)
{
    retail_assert(binary_schema != nullptr, "binary_schema argument should not be null.");

    const flatbuffers::Table* root_table = nullptr;
    auto_field_cache_t auto_field_cache;
    field_cache_t local_field_cache;
    const reflection::Field* field = nullptr;
    const flatbuffers::VectorOfAny* field_value = nullptr;
    vector<uint8_t> updatable_serialized_data(serialized_data, serialized_data + serialized_data_size);

    get_table_field_array_information(
        type_id, updatable_serialized_data.data(), binary_schema, binary_schema_size, field_position,
        root_table, auto_field_cache, local_field_cache, field, field_value);

    retail_assert(array_index < field_value->size(), "Attempt to index array is out-of-bounds.");
    retail_assert(
        field->type()->element() == reflection::String && value.type == reflection::String,
        "Attempt to set value of incorrect type");

    const reflection::Schema* schema = reflection::GetSchema(binary_schema);
    if (schema == nullptr)
    {
        throw invalid_schema();
    }

    string new_field_element_value(value.hold.string_value);

    auto field_element_value
        = flatbuffers::GetAnyVectorElemPointer<const flatbuffers::String>(field_value, array_index);
    if (field_element_value == nullptr)
    {
        throw invalid_serialized_data();
    }

    flatbuffers::SetString(
        *schema,
        new_field_element_value,
        field_element_value,
        &updatable_serialized_data);

    return updatable_serialized_data;
}

} // namespace payload_types
} // namespace db
} // namespace gaia
