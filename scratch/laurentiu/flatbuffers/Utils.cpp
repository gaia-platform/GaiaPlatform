/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "Utils.h"

#include <fstream>

using namespace std;

void LoadFileData(const char* filename, char*& data, int& length)
{
    ifstream infile;
    infile.open(filename, ios::binary | ios::in);
    infile.seekg(0, ios::end);
    length = infile.tellg();

    infile.seekg(0, ios::beg);
    data = new char[length];
    infile.read(data, length);
    infile.close();
}
