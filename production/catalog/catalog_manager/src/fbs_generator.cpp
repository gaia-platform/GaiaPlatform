/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include <string>
#include <algorithm>
#include "gaia_catalog.h"
#include "fbs_generator.hpp"
#include "flatbuffers/idl.h"
#include "retail_assert.hpp"

namespace gaia {
namespace catalog {

/**
* Helper functions
**/

/**
 * Return the position of chr within base64_encode()
 */
static unsigned int pos_of_char(const unsigned char chr) {
    if (chr >= 'A' && chr <= 'Z') {
        return chr - 'A';
    } else if (chr >= 'a' && chr <= 'z') {
        return chr - 'a' + ('Z' - 'A') + 1;
    } else if (chr >= '0' && chr <= '9') {
        return chr - '0' + ('Z' - 'A') + ('z' - 'a') + 2;
    } else if (chr == '+' || chr == '-') {
        return 62;
    } else if (chr == '/' || chr == '_') {
        return 63;
    }

    retail_assert(false, "Unknown base64 char!");
    return 0;
}

/**
 * base64 decode function adapted from the following implementation.
 * https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp/
 *
 * This is for temporary workaround to decode string into binary buffer before EDC support arrays.
 * Do not use it elsewhere.
 **/
static string base64_decode(string encoded_string) {
    size_t length_of_string = encoded_string.length();
    if (!length_of_string) {
        return string("");
    }

    size_t input_length = length_of_string;
    size_t pos = 0;

    // The approximate length (bytes) of the decoded string might be one ore
    // two bytes smaller, depending on the amount of trailing equal signs
    // in the encoded string. This approximation is needed to reserve
    // enough space in the string to be returned.
    size_t approx_length_of_decoded_string = length_of_string / 4 * 3;
    string ret;
    ret.reserve(approx_length_of_decoded_string);

    while (pos < input_length) {
        unsigned int pos_of_char_1 = pos_of_char(encoded_string[pos + 1]);
        ret.push_back(static_cast<std::string::value_type>(((pos_of_char(encoded_string[pos + 0])) << 2) + ((pos_of_char_1 & 0x30) >> 4)));
        if (encoded_string[pos + 2] != '=') {
            unsigned int pos_of_char_2 = pos_of_char(encoded_string[pos + 2]);
            ret.push_back(static_cast<std::string::value_type>(((pos_of_char_1 & 0x0f) << 4) + ((pos_of_char_2 & 0x3c) >> 2)));
            if (encoded_string[pos + 3] != '=') {
                ret.push_back(static_cast<std::string::value_type>(((pos_of_char_2 & 0x03) << 6) + pos_of_char(encoded_string[pos + 3])));
            }
        }
        pos += 4;
    }
    return ret;
}

/**
 * base64 encode function adapted from the following implementation.
 * https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp/
 *
 * This is for temporary workaround to encode binary into string before EDC support arrays.
 * Do not use it elsewhere.
 **/
static string base64_encode(uint8_t const *bytes_to_encode, uint32_t in_len) {
    uint32_t len_encoded = (in_len + 2) / 3 * 4;
    unsigned char trailing_char = '=';
    static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                       "abcdefghijklmnopqrstuvwxyz"
                                       "0123456789"
                                       "+/";

    string ret;
    ret.reserve(len_encoded);

    unsigned int pos = 0;
    while (pos < in_len) {
        ret.push_back(base64_chars[(bytes_to_encode[pos + 0] & 0xfc) >> 2]);
        if (pos + 1 < in_len) {
            ret.push_back(base64_chars[((bytes_to_encode[pos + 0] & 0x03) << 4) + ((bytes_to_encode[pos + 1] & 0xf0) >> 4)]);
            if (pos + 2 < in_len) {
                ret.push_back(base64_chars[((bytes_to_encode[pos + 1] & 0x0f) << 2) + ((bytes_to_encode[pos + 2] & 0xc0) >> 6)]);
                ret.push_back(base64_chars[bytes_to_encode[pos + 2] & 0x3f]);
            } else {
                ret.push_back(base64_chars[(bytes_to_encode[pos + 1] & 0x0f) << 2]);
                ret.push_back(trailing_char);
            }
        } else {
            ret.push_back(base64_chars[(bytes_to_encode[pos + 0] & 0x03) << 4]);
            ret.push_back(trailing_char);
            ret.push_back(trailing_char);
        }
        pos += 3;
    }
    return ret;
}

/**
 * Get the data type name for fbs
 *
 * @param catalog data type
 * @return fbs data type name
 * @throw unknown_data_type
 */
static string get_data_type_name(data_type_t data_type) {
    switch (data_type) {
    case data_type_t::e_bool:
        return "bool";
    case data_type_t::e_int8:
        return "int8";
    case data_type_t::e_uint8:
        return "uint8";
    case data_type_t::e_int16:
        return "int16";
    case data_type_t::e_uint16:
        return "uint16";
    case data_type_t::e_int32:
        return "int32";
    case data_type_t::e_uint32:
        return "uint32";
    case data_type_t::e_int64:
        return "int64";
    case data_type_t::e_uint64:
        return "uint64";
    case data_type_t::e_float32:
        return "float32";
    case data_type_t::e_float64:
        return "float64";
    case data_type_t::e_string:
        return "string";
    default:
        throw ddl::unknown_data_type();
    }
}

static string generate_field_fbs(const string &name, const string &type, int count) {
    if (count == 1) {
        return name + ":" + type;
    } else if (count == 0) {
        return name + ":[" + type + "]";
    } else {
        return name + ":[" + type + ":" + to_string(count) + "]";
    }
}

static string generate_field_fbs(const gaia_field_t &field) {
    string name{field.name()};
    string type{get_data_type_name(static_cast<data_type_t>(field.type()))};
    return generate_field_fbs(name, type, field.repeated_count());
}

/**
 * Public interfaces
 **/
ddl::unknown_data_type::unknown_data_type() {
    m_message = "Unknown data type.";
}

string generate_fbs(gaia_id_t table_id) {
    string fbs;
    gaia::db::begin_transaction();
    gaia_table_t table = gaia_table_t::get(table_id);
    string table_name{table.name()};
    fbs += "table " + table_name + "{\n";
    for (gaia_id_t field_id : list_fields(table_id)) {
        gaia_field_t field = gaia_field_t::get(field_id);
        fbs += "\t" + generate_field_fbs(field) + ";\n";
    }
    fbs += "}\n";
    fbs += "root_type " + table_name + ";";
    gaia::db::commit_transaction();
    return fbs;
}

string generate_fbs() {
    string fbs;
    gaia::db::begin_transaction();
    for (gaia_id_t table_id : list_tables()) {
        gaia_table_t table = gaia_table_t::get(table_id);
        fbs += "table " + string(table.name()) + "{\n";
        for (gaia_id_t field_id : list_fields(table_id)) {
            gaia_field_t field = gaia_field_t::get(field_id);
            fbs += "\t" + generate_field_fbs(field) + ";\n";
        }
        fbs += "}\n\n";
    }
    gaia::db::commit_transaction();
    return fbs;
}

string generate_fbs(const string &table_name, const ddl::field_def_list_t &fields) {
    string fbs;
    fbs += "table " + table_name + "{";
    for (auto &field : fields) {
        if (field->type == data_type_t::e_references) {
            continue;
        }
        string field_fbs = generate_field_fbs(
            field->name,
            get_data_type_name(field->type),
            field->length);
        fbs += field_fbs + ";";
    }
    fbs += "}";
    fbs += "root_type " + table_name + ";";
    return fbs;
}

string generate_bfbs(const string &fbs) {
    flatbuffers::Parser fbs_parser;
    bool parsing_result = fbs_parser.Parse(fbs.c_str());
    retail_assert(parsing_result == true, "Invalid FlatBuffers schema!");
    fbs_parser.Serialize();
    return base64_encode(fbs_parser.builder_.GetBufferPointer(), fbs_parser.builder_.GetSize());
}

string get_bfbs(gaia_id_t table_id) {
    bool is_transaction_owner = !gaia::db::is_transaction_active();
    if (is_transaction_owner) {
        gaia::db::begin_transaction();
    }
    gaia_table_t table = gaia_table_t::get(table_id);
    string base64_binary_schema = table.binary_schema();
    if (is_transaction_owner) {
        gaia::db::commit_transaction();
    }
    return base64_decode(base64_binary_schema);
}

} // namespace catalog
} // namespace gaia
