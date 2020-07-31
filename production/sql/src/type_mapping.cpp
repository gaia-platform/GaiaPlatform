/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "type_mapping.hpp"

// Valid options for gaia_fdw.
const gaia_fdw_option_t valid_options[] = {
    // Sentinel.
    {NULL, InvalidOid, NULL}
};

Datum flatbuffers_string_to_text_datum(flatbuffers_string_t str)
{
    size_t str_len = flatbuffers_string_len(str);
    size_t text_len = str_len + VARHDRSZ;
    text *t = (text *)palloc(text_len);
    SET_VARSIZE(t, text_len);
    memcpy(VARDATA(t), str, str_len);
    return CStringGetDatum(t);
}

bool is_valid_option(const char *option, const char *value, Oid context)
{
    const gaia_fdw_option_t *opt;
    for (opt = valid_options; opt->name; opt++) {
        if (context == opt->context && strcmp(opt->name, option) == 0) {
            // Invoke option handler callback.
            opt->handler(option, value, context);
            return true;
        }
    }
    return false;
}
