/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#ifndef _UTILS_H_
#define _UTILS_H_

// Load data from a file. Caller is responsible for deallocating the buffer memory.
void LoadFileData(const char* filename, char*& data, int& length);

#endif // _UTILS_H_
