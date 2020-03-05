/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "Experiment.h"

#include <iostream>

JNIEXPORT jlong JNICALL Java_Experiment_addIntegers(
    JNIEnv*, jobject, jint first, jint second)
{
    std::cout << "C++ addIntegers(): Received values " << first << " and " << second << "." << std::endl;
    return (long)first + (long)second;
}

JNIEXPORT jstring JNICALL Java_Experiment_formalizeName(
    JNIEnv* env, jobject, jstring name, jboolean isFemale)
{
    const char* nameCharPointer = env->GetStringUTFChars(name, NULL);
    std::string title;

    if (isFemale)
    {
        title = "Ms. ";
    }
    else
    {
        title = "Mr. ";
    }
 
    std::string fullName = title + nameCharPointer;
    return env->NewStringUTF(fullName.c_str());
}
