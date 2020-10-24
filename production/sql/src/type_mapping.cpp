/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "type_mapping.hpp"

Datum flatbuffers_string_to_text_datum(flatbuffers_string_t str)
{
    size_t str_len = flatbuffers_string_len(str);
    size_t text_len = str_len + VARHDRSZ;
    text* t = reinterpret_cast<text*>(palloc(text_len));

    SET_VARSIZE(t, text_len); // NOLINT (macro expansion)
    memcpy(VARDATA(t), str, str_len); // NOLINT (macro expansion)

    return CStringGetDatum(t); // NOLINT (macro expansion)
}
