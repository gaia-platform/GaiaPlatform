/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "generated/blobv1_generated.h"
#include "generated/blobv2_generated.h"

#include "Utils.h"

using namespace std;
using namespace flatbuffers;

void TestVersions();

int main(void)
{
    TestVersions();

    return 0;
}

void DumpBlobV1Attributes(const BlobVersion1::BlobV1& blob)
{
    cout << endl << "BlobV1 content is:" << endl;
    cout << "  color = " << blob.color() << endl;
    cout << "  id = " << blob.id() << endl;
    cout << "  version = " << blob.version() << endl;
    cout << "  year = " << blob.year() << endl << endl;
}

void DumpBlobV2Attributes(const BlobVersion2::BlobV2& blob)
{
    cout << endl << "BlobV2 content is:" << endl;
    cout << "  color = " << blob.color() << endl;
    cout << "  id = " << blob.id() << endl;
    cout << "  version = deprecated, cannot read" << endl;
    cout << "  year = " << blob.year() << endl;
    cout << "  month = " << (int)blob.month() << endl;
    cout << "  day = " << (int)blob.day() << endl << endl;
}

void LoadAndProcessData(const char* filename)
{
    char* data = nullptr;
    int length = 0;

    cout << "Loading " << filename << "..." << endl;
    LoadFileData(filename, data, length);

    cout << "File size is: " << length << "." << endl << endl;

    cout << "Deserializing data as BlobV1..." << endl;
    auto blobV1 = BlobVersion1::GetBlobV1(data);

    DumpBlobV1Attributes(*blobV1);

    cout << "Deserializing data as mutable BlobV1 and updating version to 7..." << endl;
    auto mutableBlobV1 = BlobVersion1::GetMutableBlobV1(data);

    bool mutationResult = mutableBlobV1->mutate_version(7);
    if (!mutationResult)
    {
        cout << "Version update failed!" << endl;
    }
    DumpBlobV1Attributes(*mutableBlobV1);

    cout << "Deserializing data as BlobV2..." << endl;
    auto blobV2 = BlobVersion2::GetBlobV2(data);

    DumpBlobV2Attributes(*blobV2);

    cout << "Deserializing data as mutable BlobV2 and updating year to 2030..." << endl;
    auto mutableBlobV2 = BlobVersion2::GetMutableBlobV2(data);

    mutationResult = mutableBlobV2->mutate_year(2030);
    if (!mutationResult)
    {
        cout << "Year update failed!" << endl;
    }
    DumpBlobV2Attributes(*mutableBlobV2);

    cout << "Deserializing data again as BlobV1..." << endl;
    blobV1 = BlobVersion1::GetBlobV1(data);

    DumpBlobV1Attributes(*blobV1);

    delete[] data;
}

void LoadBlobV1Data()
{
    cout << endl << ">>> Part 1: Deserialize BlobV1. <<<" << endl << endl;

    LoadAndProcessData("blobv1_data.bin");
}

void LoadBlobV2Data()
{
    cout << endl << ">>> Part 2: Deserialize BlobV2. <<<" << endl << endl;

    LoadAndProcessData("blobv2_data1.bin");
}

void LoadBlobV2DataWithV1Fields()
{
    cout << endl << ">>> Part 3: Deserialize BlobV2 that includes V1 fields. <<<" << endl << endl;

    LoadAndProcessData("blobv2_data2.bin");
}

void TestVersions()
{
     LoadBlobV1Data();

     LoadBlobV2Data();

     LoadBlobV2DataWithV1Fields();
}
