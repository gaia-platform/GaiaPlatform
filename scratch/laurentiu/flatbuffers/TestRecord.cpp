/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "generated/record_generated.h"

using namespace std;
using namespace flatbuffers;
using namespace Flatbuffers::Record;

void TestRecord();

int main(void)
{
    TestRecord();

    return 0;
}

void CreateRecord(uint8_t*& buf, int& size, uint64_t data, bool force_defaults)
{
    FlatBufferBuilder builder(1024);

    builder.ForceDefaults(force_defaults);

    auto record = CreateRecord(
        builder,
        12302652060373662634U,
        data,
        12302652060373662634U);

    builder.Finish(record);

    // This must be called after `Finish()`.
    buf = builder.GetBufferPointer();
    // Returns the size of the buffer that
    // `GetBufferPointer()` points to.
    size = builder.GetSize();

    cout << "ForceDefaults was set to: " << (force_defaults ? "true" : "false") << "." << endl;
    cout << "Data is set to default: " << ((data == 15) ? "true" : "false") << "." << endl;
    cout << "Record buffer size is: " << size << "." << endl << endl;
}

void TestRecord()
{
    uint8_t* buf = nullptr;
    int size = 0;

    CreateRecord(buf, size, 15, false);
    CreateRecord(buf, size, 31, false);
    CreateRecord(buf, size, 15, true);
    CreateRecord(buf, size, 31, true);
}
