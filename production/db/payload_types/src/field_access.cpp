////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "field_access.hpp"

#include <sstream>

#include "gaia_internal/common/assert.hpp"

using namespace std;
using namespace gaia::common;

namespace gaia
{
namespace db
{
namespace payload_types
{

static constexpr char c_message_attempt_to_set_value_of_incorrect_type[] = "Attempt to set value of incorrect type";
static constexpr char c_message_array_index_out_of_bounds[] = "Attempt to index array is out-of-bounds.";

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
    string_stream << "Cannot handle field type: '" << field_type << "'.";
    m_message = string_stream.str();
}

cannot_set_null_string_value::cannot_set_null_string_value()
{
    m_message = "Setting null string values is not supported!";
}

cannot_update_null_string_value::cannot_update_null_string_value()
{
    m_message = "Updating null string values is not supported!";
}

// Data validation method.
bool verify_data_schema(
    const uint8_t* serialized_data,
    size_t serialized_data_size,
    const uint8_t* binary_schema)
{
    ASSERT_PRECONDITION(serialized_data != nullptr, "'serialized_data argument' should not be null.");
    ASSERT_PRECONDITION(binary_schema != nullptr, "'binary_schema argument' should not be null.");

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
// and then it retrieves the reflection::Field information from the type's binary schema.
void get_table_field_information(
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    field_position_t field_position,
    const flatbuffers::Table*& root_table,
    const reflection::Field*& field)
{
    ASSERT_PRECONDITION(serialized_data != nullptr, "'serialized_data' argument should not be null.");

    // First, we parse the serialized data to get its root object.
    root_table = flatbuffers::GetAnyRoot(serialized_data);
    if (root_table == nullptr)
    {
        throw invalid_serialized_data();
    }

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

    // Get the collection of fields and lookup our field.
    //
    // NOTE: If this ends up being a bottleneck, we can bring back the caching of these data.
    auto fields = root_type->fields();
    for (size_t i = 0; i < fields->size(); ++i)
    {
        field = fields->Get(i);
        if (field->id() == field_position)
        {
            return;
        }
    }

    throw invalid_field_position(field_position);
}

// Another helper for the methods that access array-type fields.
// This helper also retrieves a VectorOfAny pointer
// that allows operating on the array.
void get_table_field_array_information(
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    field_position_t field_position,
    const flatbuffers::Table*& root_table,
    const reflection::Field*& field,
    const flatbuffers::VectorOfAny*& field_value)
{
    get_table_field_information(
        serialized_data,
        binary_schema,
        field_position,
        root_table,
        field);

    if (field->type()->base_type() != reflection::Vector)
    {
        throw unhandled_field_type(field->type()->base_type());
    }

    field_value = GetFieldAnyV(*root_table, *field);
}

bool are_field_values_equal(
    const uint8_t* first_serialized_data,
    const uint8_t* second_serialized_data,
    const uint8_t* binary_schema,
    field_position_t field_position)
{
    const flatbuffers::Table* first_root_table = nullptr;
    const reflection::Field* field = nullptr;

    get_table_field_information(
        first_serialized_data,
        binary_schema,
        field_position,
        first_root_table,
        field);

    const flatbuffers::Table* second_root_table = flatbuffers::GetAnyRoot(second_serialized_data);
    if (second_root_table == nullptr)
    {
        throw invalid_serialized_data();
    }

    // Compare field values according to their type.
    if (flatbuffers::IsInteger(field->type()->base_type()))
    {
        bool is_first_value_null = (first_root_table->GetAddressOf(field->offset()) == nullptr);
        bool is_second_value_null = (second_root_table->GetAddressOf(field->offset()) == nullptr);

        if (is_first_value_null || is_second_value_null)
        {
            return is_first_value_null && is_second_value_null;
        }
        else
        {
            int64_t first_value = flatbuffers::GetAnyFieldI(*first_root_table, *field);
            int64_t second_value = flatbuffers::GetAnyFieldI(*second_root_table, *field);

            return first_value == second_value;
        }
    }
    else if (flatbuffers::IsFloat(field->type()->base_type()))
    {
        bool is_first_value_null = (first_root_table->GetAddressOf(field->offset()) == nullptr);
        bool is_second_value_null = (second_root_table->GetAddressOf(field->offset()) == nullptr);

        if (is_first_value_null || is_second_value_null)
        {
            return is_first_value_null && is_second_value_null;
        }
        else
        {
            double first_value = flatbuffers::GetAnyFieldF(*first_root_table, *field);
            double second_value = flatbuffers::GetAnyFieldF(*second_root_table, *field);

            return first_value == second_value;
        }
    }
    else if (field->type()->base_type() == reflection::String)
    {
        const flatbuffers::String* first_value = flatbuffers::GetFieldS(*first_root_table, *field);
        const flatbuffers::String* second_value = flatbuffers::GetFieldS(*second_root_table, *field);

        if (first_value == nullptr || second_value == nullptr)
        {
            return first_value == second_value;
        }
        else
        {
            return strcmp(first_value->c_str(), second_value->c_str()) == 0;
        }
    }
    else if (field->type()->base_type() == reflection::Vector)
    {
        flatbuffers::VectorOfAny* first_value = flatbuffers::GetFieldAnyV(*first_root_table, *field);
        flatbuffers::VectorOfAny* second_value = flatbuffers::GetFieldAnyV(*second_root_table, *field);

        if (first_value == nullptr || second_value == nullptr)
        {
            return first_value == second_value;
        }
        else if (first_value->size() != second_value->size())
        {
            return false;
        }
        else
        {
            return memcmp(first_value->Data(), second_value->Data(), first_value->size()) == 0;
        }
    }
    else
    {
        throw unhandled_field_type(field->type()->base_type());
    }
}

data_holder_t get_field_value(
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    field_position_t field_position)
{
    const flatbuffers::Table* root_table = nullptr;
    const reflection::Field* field = nullptr;

    get_table_field_information(
        serialized_data,
        binary_schema,
        field_position,
        root_table,
        field);

    // Read field value according to its type.
    data_holder_t result;
    result.type = field->type()->base_type();

    if (flatbuffers::IsInteger(field->type()->base_type()))
    {
        result.is_null = (root_table->GetAddressOf(field->offset()) == nullptr);
        if (!result.is_null)
        {
            result.hold.integer_value = flatbuffers::GetAnyFieldI(*root_table, *field);
        }
    }
    else if (flatbuffers::IsFloat(field->type()->base_type()))
    {
        result.is_null = (root_table->GetAddressOf(field->offset()) == nullptr);
        if (!result.is_null)
        {
            result.hold.float_value = flatbuffers::GetAnyFieldF(*root_table, *field);
        }
    }
    else if (field->type()->base_type() == reflection::String)
    {
        const flatbuffers::String* field_value = flatbuffers::GetFieldS(*root_table, *field);

        // For null strings, the field_value will come back as nullptr.
        result.is_null = (field_value == nullptr);
        if (!result.is_null)
        {
            result.hold.string_value = field_value->c_str();
        }
    }
    else
    {
        throw unhandled_field_type(field->type()->base_type());
    }

    return result;
}

bool set_field_value(
    uint8_t* serialized_data,
    const uint8_t* binary_schema,
    field_position_t field_position,
    const data_holder_t& value)
{
    const flatbuffers::Table* const_root_table = nullptr;
    const reflection::Field* field = nullptr;

    get_table_field_information(
        serialized_data,
        binary_schema,
        field_position,
        const_root_table,
        field);

    ASSERT_PRECONDITION(
        (flatbuffers::IsInteger(field->type()->base_type()) && flatbuffers::IsInteger(value.type))
            || (flatbuffers::IsFloat(field->type()->base_type()) && flatbuffers::IsFloat(value.type)),
        c_message_attempt_to_set_value_of_incorrect_type);

    // We need to update the root_table, so we need to remove the const qualifier.
    auto root_table = const_cast<flatbuffers::Table*>(const_root_table); // NOLINT (safe const_cast)

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

void set_field_value(
    vector<uint8_t>& serialized_data,
    const uint8_t* binary_schema,
    field_position_t field_position,
    const data_holder_t& value)
{
    ASSERT_PRECONDITION(binary_schema != nullptr, "'binary_schema' argument should not be null.");
    ASSERT_PRECONDITION(value.type == reflection::String, c_message_attempt_to_set_value_of_incorrect_type);

    if (value.hold.string_value == nullptr)
    {
        throw cannot_set_null_string_value();
    }

    const flatbuffers::Table* root_table = nullptr;
    const reflection::Field* field = nullptr;

    get_table_field_information(
        serialized_data.data(),
        binary_schema,
        field_position,
        root_table,
        field);

    ASSERT_PRECONDITION(
        field->type()->base_type() == reflection::String, c_message_attempt_to_set_value_of_incorrect_type);

    const reflection::Schema* schema = reflection::GetSchema(binary_schema);
    if (schema == nullptr)
    {
        throw invalid_schema();
    }

    string new_field_value(value.hold.string_value);

    const flatbuffers::String* field_value = flatbuffers::GetFieldS(*root_table, *field);
    if (field_value == nullptr)
    {
        throw cannot_update_null_string_value();
    }

    flatbuffers::SetString(
        *schema,
        new_field_value,
        field_value,
        &serialized_data);
}

vector<uint8_t> set_field_value(
    const uint8_t* serialized_data,
    size_t serialized_data_size,
    const uint8_t* binary_schema,
    field_position_t field_position,
    const data_holder_t& value)
{
    vector<uint8_t> updatable_serialized_data(serialized_data, serialized_data + serialized_data_size);

    set_field_value(
        updatable_serialized_data,
        binary_schema,
        field_position,
        value);

    return updatable_serialized_data;
}

size_t get_field_array_size(
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    field_position_t field_position)
{
    const flatbuffers::Table* root_table = nullptr;
    const reflection::Field* field = nullptr;
    const flatbuffers::VectorOfAny* field_value = nullptr;

    get_table_field_array_information(
        serialized_data,
        binary_schema,
        field_position,
        root_table,
        field,
        field_value);

    if (field_value == nullptr)
    {
        return std::numeric_limits<size_t>::max();
    }
    else
    {
        // Note: size() returns a uint32_t, rather than a size_t.
        return field_value->size();
    }
}

void set_field_array_size(
    vector<uint8_t>& serialized_data,
    const uint8_t* binary_schema,
    field_position_t field_position,
    size_t new_size)
{
    ASSERT_PRECONDITION(binary_schema != nullptr, "'binary_schema' argument should not be null.");

    const flatbuffers::Table* root_table = nullptr;
    const reflection::Field* field = nullptr;
    const flatbuffers::VectorOfAny* field_value = nullptr;

    get_table_field_array_information(
        serialized_data.data(),
        binary_schema,
        field_position,
        root_table,
        field,
        field_value);

    if (new_size == field_value->size())
    {
        // No change is needed.
        return;
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
        &serialized_data);
}

vector<uint8_t> set_field_array_size(
    const uint8_t* serialized_data,
    size_t serialized_data_size,
    const uint8_t* binary_schema,
    field_position_t field_position,
    size_t new_size)
{
    vector<uint8_t> updatable_serialized_data(serialized_data, serialized_data + serialized_data_size);

    set_field_array_size(
        updatable_serialized_data,
        binary_schema,
        field_position,
        new_size);

    return updatable_serialized_data;
}

data_holder_t get_field_array_element(
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    field_position_t field_position,
    size_t array_index)
{
    const flatbuffers::Table* root_table = nullptr;
    const reflection::Field* field = nullptr;
    const flatbuffers::VectorOfAny* field_value = nullptr;

    get_table_field_array_information(
        serialized_data,
        binary_schema,
        field_position,
        root_table,
        field,
        field_value);

    ASSERT_PRECONDITION(array_index < field_value->size(), c_message_array_index_out_of_bounds);

    // Read element value according to its type.
    data_holder_t result;
    result.type = field->type()->element();
    if (flatbuffers::IsInteger(field->type()->element()))
    {
        result.is_null = false;
        result.hold.integer_value = flatbuffers::GetAnyVectorElemI(
            field_value, field->type()->element(), array_index);
    }
    else if (flatbuffers::IsFloat(field->type()->element()))
    {
        result.is_null = false;
        result.hold.float_value = flatbuffers::GetAnyVectorElemF(
            field_value, field->type()->element(), array_index);
    }
    else if (field->type()->element() == reflection::String)
    {
        const auto field_element_value
            = flatbuffers::GetAnyVectorElemPointer<const flatbuffers::String>(field_value, array_index);
        if (field_element_value == nullptr)
        {
            // Unlike in the string case, when we were calling GetFieldS(),
            // GetAnyVectorElemPointer() should not be able to return nullptr.
            throw invalid_serialized_data();
        }

        result.is_null = false;
        result.hold.string_value = field_element_value->c_str();
    }
    else
    {
        throw unhandled_field_type(field->type()->element());
    }

    return result;
}

void set_field_array_element(
    uint8_t* serialized_data,
    const uint8_t* binary_schema,
    field_position_t field_position,
    size_t array_index,
    const data_holder_t& value)
{
    const flatbuffers::Table* const_root_table = nullptr;
    const reflection::Field* field = nullptr;
    const flatbuffers::VectorOfAny* const_field_value = nullptr;

    get_table_field_array_information(
        serialized_data,
        binary_schema,
        field_position,
        const_root_table,
        field,
        const_field_value);

    ASSERT_PRECONDITION(array_index < const_field_value->size(), c_message_array_index_out_of_bounds);
    ASSERT_PRECONDITION(
        (flatbuffers::IsInteger(field->type()->element()) && flatbuffers::IsInteger(value.type))
            || (flatbuffers::IsFloat(field->type()->element()) && flatbuffers::IsFloat(value.type)),
        c_message_attempt_to_set_value_of_incorrect_type);

    // We need to update the serialization, so we need to remove the const qualifier.
    auto field_value = const_cast<flatbuffers::VectorOfAny*>(const_field_value); // NOLINT (safe const_cast)

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

void set_field_array_element(
    vector<uint8_t>& serialized_data,
    const uint8_t* binary_schema,
    field_position_t field_position,
    size_t array_index,
    const data_holder_t& value)
{
    ASSERT_PRECONDITION(binary_schema != nullptr, "'binary_schema' argument should not be null.");
    ASSERT_PRECONDITION(value.type == reflection::String, c_message_attempt_to_set_value_of_incorrect_type);

    if (value.hold.string_value == nullptr)
    {
        throw cannot_set_null_string_value();
    }

    const flatbuffers::Table* root_table = nullptr;
    const reflection::Field* field = nullptr;
    const flatbuffers::VectorOfAny* field_value = nullptr;

    get_table_field_array_information(
        serialized_data.data(),
        binary_schema,
        field_position,
        root_table,
        field,
        field_value);

    ASSERT_PRECONDITION(array_index < field_value->size(), c_message_array_index_out_of_bounds);
    ASSERT_PRECONDITION(field->type()->element() == reflection::String, c_message_attempt_to_set_value_of_incorrect_type);

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
        // Unlike in the string case, when we were calling GetFieldS(),
        // GetAnyVectorElemPointer() should not be able to return nullptr.
        throw invalid_serialized_data();
    }

    flatbuffers::SetString(
        *schema,
        new_field_element_value,
        field_element_value,
        &serialized_data);
}

std::vector<uint8_t> set_field_array_element(
    const uint8_t* serialized_data,
    size_t serialized_data_size,
    const uint8_t* binary_schema,
    field_position_t field_position,
    size_t array_index,
    const data_holder_t& value)
{
    vector<uint8_t> updatable_serialized_data(serialized_data, serialized_data + serialized_data_size);

    set_field_array_element(
        updatable_serialized_data,
        binary_schema,
        field_position,
        array_index,
        value);

    return updatable_serialized_data;
}

} // namespace payload_types
} // namespace db
} // namespace gaia
