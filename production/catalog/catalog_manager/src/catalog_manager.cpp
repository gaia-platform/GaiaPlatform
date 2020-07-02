/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "catalog_manager.hpp"
#include "gaia_exception.hpp"
#include "fbs_generator.hpp"
#include "retail_assert.hpp"
#include <array>

using namespace gaia::catalog;

/**
 * Catalog public APIs
 **/
gaia_id_t gaia::catalog::create_table(const string &name,
    const vector<ddl::field_definition_t *> &fields) {
    return catalog_manager_t::get().create_table(name, fields);
}

const set<gaia_id_t> &list_tables() {
    return catalog_manager_t::get().list_tables();
}

const vector<gaia_id_t> &list_fields(gaia_id_t table_id) {
    return catalog_manager_t::get().list_fields(table_id);
}

/**
 * Return the position of chr within base64_encode()
 */
static unsigned int pos_of_char(const unsigned char chr) {
    if (chr >= 'A' && chr <= 'Z')
        return chr - 'A';
    else if (chr >= 'a' && chr <= 'z')
        return chr - 'a' + ('Z' - 'A') + 1;
    else if (chr >= '0' && chr <= '9')
        return chr - '0' + ('Z' - 'A') + ('z' - 'a') + 2;
    else if (chr == '+' || chr == '-')
        return 62; // Be liberal with input and accept both url ('-') and non-url ('+') base 64 characters (
    else if (chr == '/' || chr == '_')
        return 63; // Ditto for '/' and '_'

    throw "If input is correct, this line should never be reached.";
}

/**
 * base64 decode function adapted from the following implementation.
 * https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp/
 *
 * This is for temoprary workdaround to decode string into binary buffer before EDC support arrays.
 **/
static pair<uint8_t *, uint32_t> base64_decode(string encoded_string) {
    size_t length_of_string = encoded_string.length();
    if (!length_of_string)
        return {nullptr, 0};

    size_t in_len = length_of_string;
    size_t pos = 0;

    //
    // The approximate length (bytes) of the decoded string might be one ore
    // two bytes smaller, depending on the amount of trailing equal signs
    // in the encoded string. This approximation is needed to reserve
    // enough space in the string to be returned.
    //
    size_t approx_length_of_decoded_string = length_of_string / 4 * 3;
    uint8_t *buf = new uint8_t[approx_length_of_decoded_string + 2];
    uint32_t i = 0;

    while (pos < in_len) {

        unsigned int pos_of_char_1 = pos_of_char(encoded_string[pos + 1]);

        buf[i++] = ((pos_of_char(encoded_string[pos + 0])) << 2) + ((pos_of_char_1 & 0x30) >> 4);

        if (encoded_string[pos + 2] != '=') {

            unsigned int pos_of_char_2 = pos_of_char(encoded_string[pos + 2]);
            buf[i++] = ((pos_of_char_1 & 0x0f) << 4) + ((pos_of_char_2 & 0x3c) >> 2);

            if (encoded_string[pos + 3] != '=') {
                buf[i++] = ((pos_of_char_2 & 0x03) << 6) + pos_of_char(encoded_string[pos + 3]);
            }
        }

        pos += 4;
    }

    return {buf, i + 1};
}

pair<uint8_t *, uint32_t> get_bfbs(gaia_id_t table_id) {
    gaia::db::begin_transaction();
    unique_ptr<Gaia_table> table{Gaia_table::get_row_by_id(table_id)};
    gaia::db::commit_transaction();
    return base64_decode(table->binary_schema());
}

/**
 * Class methods
 **/
catalog_manager_t &catalog_manager_t::get() {
    static catalog_manager_t s_instance;
    return s_instance;
}

gaia_id_t catalog_manager_t::create_table(const string &name,
    const vector<ddl::field_definition_t *> &fields) {
    if (m_table_names.find(name) != m_table_names.end()) {
        throw table_already_exists(name);
    }

    string bfbs{generate_bfbs(generate_fbs(name, fields))};

    gaia::db::begin_transaction();
    gaia_id_t table_id = Gaia_table::insert_row(
        name.c_str(),               // name
        false,                      // is_log
        gaia_trim_action_type_NONE, // trim_action
        0,                          // max_rows
        0,                          // max_size
        0,                          // max_seconds
        bfbs.c_str()                // bfbs
    );

    uint16_t position = 0;
    vector<gaia_id_t> field_ids;
    for (ddl::field_definition_t *field : fields) {
        gaia_id_t field_id = Gaia_field::insert_row(
            field->name.c_str(),            // name
            table_id,                       // table_id
            to_gaia_data_type(field->type), // type
            field->length,                  // repeated_count
            ++position,                     // position
            true,                           // required
            false,                          // deprecated
            false,                          // active
            true,                           // nullable
            false,                          // has_default
            ""                              // default value
        );
        field_ids.push_back(field_id);
    }
    gaia::db::commit_transaction();

    m_table_names[name] = table_id;
    m_table_ids.insert(table_id);
    m_table_fields[table_id] = move(field_ids);
    return table_id;
}

const set<gaia_id_t> &catalog_manager_t::list_tables() const {
    return m_table_ids;
}

const vector<gaia_id_t> &catalog_manager_t::list_fields(gaia_id_t table_id) const {
    return m_table_fields.at(table_id);
}
