/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <field_access.hpp>

#include <sstream>

using namespace std;
using namespace gaia::db::types;

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

invalid_field_position::invalid_field_position(uint16_t position)
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

invalid_serialized_field_data::invalid_serialized_field_data(uint16_t position)
{
    stringstream string_stream;
    string_stream << "Cannot deserialize data for field position: " << position << ".";
    m_message = string_stream.str();
}

void load_binary_schema_into_cache(
    field_cache_t* field_cache,
    uint8_t* binary_schema)
{
    const reflection::Schema* schema = reflection::GetSchema(binary_schema);
    if (schema == nullptr)
    {
        throw invalid_schema();
    }

    const reflection::Object* root_type = schema->root_table();
    if (root_type == nullptr)
    {
        throw missing_root_type();
    }

    auto fields = root_type->fields();

    // Lookup the field by position.
    // Unfortunately, the fields vector is not ordered by this criteria,
    // so we need to do a linear search.
    for (size_t i = 0; i < fields->Length(); i++)
    {
        const reflection::Field* current_field = fields->Get(i);
        field_cache->field_map.insert(make_pair(current_field->id(), current_field));
    }
}
