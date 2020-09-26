// basic scalar fields
template<typename T_msg>
void @TABLE_NAME@_insert(const typename T_msg& msg)
{
    auto writer = @TABLE_NAME@_writer();
    writer.@FIELD_NAME@ = msg.@FIELD_NAME@; // N times
    writer.insert_row();
}

// complex scalar fields and a reference
template<typename T_msg, typename T_gaia_edc>
void @TABLE_NAME@_insert(const typename T_msg& msg, typename T_gaia_edc& edc_object)
{
    auto writer = @TABLE_NAME@_writer();
    writer.@FIELD_NAME@ = msg.@FIELD_NAME@; // N times
    edc_object.@TABLE_NAME@_list().insert(writer.insert_row());
}

// complex scalar fields, a reference, and an index
template<typename T_msg, typename T_gaia_edc>
void @TABLE_NAME@_insert(const typename T_msg& msg, typename T_gaia_edc& edc_object, const int index)
{
    auto writer = @TABLE_NAME@_writer();
    writer.index = index;
    writer.@FIELD_NAME@ = msg.@FIELD_NAME@; // N times
    edc_object.@TABLE_NAME@_list().insert(writer.insert_row());
}

// a value, a reference, and an index
template<typename T_value, typename T_gaia_edc>
void @TABLE_NAME@_insert(const typename T_value& value, typename T_gaia_edc& edc_object, const int index)
{
    auto writer = @TABLE_NAME@_writer();
    writer.index = index;
    writer.value = value; // N times
    edc_object.@TABLE_NAME@_list().insert(writer.insert_row());
}
