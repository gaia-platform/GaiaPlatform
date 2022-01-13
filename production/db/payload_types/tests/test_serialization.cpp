/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include <flatbuffers/flatbuffers.h>
#include <gtest/gtest.h>

#include "data_holder.hpp"
#include "test_serialization_generated.h"

using namespace gaia::db;
using namespace gaia::db::payload_types;
using namespace TestSerialization;

constexpr uint64_t c_unsigned_integer_value = 7;
constexpr int64_t c_integer_value = 7;
constexpr int64_t c_negated_integer_value = -c_integer_value;

constexpr double c_float_value = -12.345;
constexpr double c_another_float_value = 67.890;

constexpr const char* c_string_value = "Hello";
constexpr const char* c_another_string_value = "Aloha";
constexpr size_t c_string_length = 5;

bool verify_serialization(const data_holder_t& from, bool optional)
{
    flatbuffers::FlatBufferBuilder fbb;
    data_write_buffer_t write_buffer(fbb);
    from.serialize(write_buffer, optional);
    auto output = write_buffer.output();

    serializationBuilder sb = serializationBuilder(fbb);
    sb.add_data(output);
    auto offset = sb.Finish();
    fbb.Finish(offset);

    auto s = flatbuffers::GetRoot<serialization>(fbb.GetBufferPointer());
    data_read_buffer_t read_buffer(*s->data());
    data_holder_t to(read_buffer, from.type, optional);

    return to.compare(from) == 0;
}

TEST(payload_types, serialization_test)
{
    data_holder_t float_value = c_float_value;
    data_holder_t signed_float_value = c_another_float_value;
    data_holder_t integer_value = c_integer_value;
    data_holder_t signed_integer_value = c_negated_integer_value;
    data_holder_t unsigned_integer_value = c_unsigned_integer_value;
    data_holder_t string_value = c_string_value;
    data_holder_t vector_value(c_another_string_value, c_string_length);

    ASSERT_TRUE(verify_serialization(float_value, false));
    ASSERT_TRUE(verify_serialization(signed_float_value, false));
    ASSERT_TRUE(verify_serialization(integer_value, false));
    ASSERT_TRUE(verify_serialization(signed_integer_value, false));
    ASSERT_TRUE(verify_serialization(unsigned_integer_value, false));
    ASSERT_TRUE(verify_serialization(string_value, false));
    ASSERT_TRUE(verify_serialization(vector_value, false));
}

TEST(payload_types, optional_serialization_test)
{
    data_holder_t float_value = c_float_value;
    data_holder_t signed_float_value = c_another_float_value;
    data_holder_t integer_value = c_integer_value;
    data_holder_t signed_integer_value = c_negated_integer_value;
    data_holder_t unsigned_integer_value = c_unsigned_integer_value;
    data_holder_t string_value = c_string_value;
    data_holder_t vector_value(c_another_string_value, c_string_length);

    ASSERT_TRUE(verify_serialization(float_value, true));
    ASSERT_TRUE(verify_serialization(signed_float_value, true));
    ASSERT_TRUE(verify_serialization(integer_value, true));
    ASSERT_TRUE(verify_serialization(signed_integer_value, true));
    ASSERT_TRUE(verify_serialization(unsigned_integer_value, true));
    ASSERT_TRUE(verify_serialization(string_value, true));
    ASSERT_TRUE(verify_serialization(vector_value, true));
}

TEST(payload_types, null_serialization_test)
{
    data_holder_t nullstring;
    nullstring.type = reflection::String;
    nullstring.is_null = true;
    nullstring.hold.string_value = nullptr;

    ASSERT_TRUE(verify_serialization(nullstring, true));

    data_holder_t nullvector;
    nullvector.type = reflection::Vector;
    nullvector.is_null = true;
    nullvector.hold.vector_value = {nullptr, 0};

    ASSERT_TRUE(verify_serialization(nullvector, true));
}

using namespace std;

TEST(payload_types, null_stuff)
{
    flatbuffers::FlatBufferBuilder fbb_doctor;
    auto db = DoctorBuilder(fbb_doctor);
    db.add_name(fbb_doctor.CreateString("Suppini"));
    auto doc_ptr = db.Finish();
    fbb_doctor.Finish(doc_ptr);

    const Doctor* doctor = GetDoctor(fbb_doctor.GetBufferPointer());

    cout << "Doctor-----" << endl;
    cout << "name: " << doctor->name()->c_str() << endl;
    cout << "age: " << doctor->age() << endl;

    flatbuffers::FlatBufferBuilder fbb_patient;
    auto pb = PatientBuilder(fbb_patient);
    auto pat_ptr = pb.Finish();
    fbb_patient.Finish(pat_ptr);

    const auto* patient = flatbuffers::GetRoot<Patient>(fbb_patient.GetBufferPointer());
    cout << "Patient-----" << endl;
    cout << "name: " << patient->name() << endl;
    cout << "age: " << patient->age().value_or(-1) << endl;
}
