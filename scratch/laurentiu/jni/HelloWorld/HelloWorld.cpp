/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "HelloWorld.h"

#include <iostream>

JNIEXPORT void JNICALL Java_HelloWorld_helloWorld(JNIEnv*, jobject)
{
    std::cout << "Hello from deep inside C++ !!!" << std::endl;
}
