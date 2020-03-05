/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "Experiment.h"

#include <iostream>

JNIEXPORT jstring JNICALL Java_Experiment_formalizeName(
    JNIEnv* env, jobject, jstring name, jboolean isFemale)
{
    std::cout << "C++ formalizeName(): Received values: " << name << " and " << (int)isFemale << "." << std::endl;

    const char* nameCharacters = env->GetStringUTFChars(name, NULL);
 
    std::string title;
    if (isFemale)
    {
        title = "Ms. ";
    }
    else
    {
        title = "Mr. ";
    }
 
    std::string fullName = title + nameCharacters;

    env->ReleaseStringUTFChars(name, nameCharacters);

    return env->NewStringUTF(fullName.c_str());
}

JNIEXPORT jlong JNICALL Java_Experiment_addIntegers(
    JNIEnv*, jobject, jint first, jint second)
{
    std::cout << "C++ addIntegers(): Received values: " << first << " and " << second << "." << std::endl;

    return (long)first + (long)second;
}

JNIEXPORT jobject JNICALL Java_Experiment_getDataContainer(
    JNIEnv* env, jobject, jstring name, jdouble value)
{
    std::cout << "C++ getDataContainer(): Received values: " << name << " and " << value << "." << std::endl;

    jclass dataContainerClass = env->FindClass("DataContainer");
    jobject newDataContainer = env->AllocObject(dataContainerClass);
     
    jfieldID nameField = env->GetFieldID(dataContainerClass , "name", "Ljava/lang/String;");
    jfieldID valueField = env->GetFieldID(dataContainerClass , "value", "D");
     
    env->SetObjectField(newDataContainer, nameField, name);
    env->SetDoubleField(newDataContainer, valueField, value);
     
    return newDataContainer;
}
 
JNIEXPORT jstring JNICALL Java_Experiment_getContainerData(
    JNIEnv* env, jobject, jobject dataContainer)
{
    std::cout << "C++ getContainerData(): Received value: " << dataContainer << "." << std::endl;

    jclass dataContainerClass = env->GetObjectClass(dataContainer);
    jmethodID methodId = env->GetMethodID(dataContainerClass, "getData", "()Ljava/lang/String;");

    jstring result = (jstring)env->CallObjectMethod(dataContainer, methodId);

    return result;
}
