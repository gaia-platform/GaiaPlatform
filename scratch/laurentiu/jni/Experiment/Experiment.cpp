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
    if (nameCharacters == NULL)
    {
        return NULL;
    }
 
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

JNIEXPORT jdoubleArray JNICALL Java_Experiment_addAndAverageIntegers(
    JNIEnv* env, jobject, jintArray values)
{
    std::cout << "C++ addAndAverageIntegers(): Received value: " << values << "." << std::endl;

    jint* intArray = env->GetIntArrayElements(values, NULL);
    if (intArray == NULL)
    {
        return NULL;
    }
    jsize countValues = env->GetArrayLength(values);

    long sum = 0;
    for (int i = 0; i < countValues; i++)
    {
        sum += intArray[i];
    }
    double average = ((double)sum) / countValues;

    env->ReleaseIntArrayElements(values, intArray, 0);

    jdouble results[] = { (double)sum, average };

    // Copy results into an array that we can return.
    jdoubleArray outputResults = env->NewDoubleArray(2);
    env->SetDoubleArrayRegion(outputResults, 0 , 2, results);

    return outputResults;
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

JNIEXPORT void JNICALL Java_Experiment_throwException(
    JNIEnv* env, jobject)
{
    std::cout << "C++ throwException()." << std::endl;

    jclass exceptionClass = env->FindClass("java/lang/IllegalArgumentException");
    if (exceptionClass != NULL)
    {
        env->ThrowNew(exceptionClass, "This exception was thrown from C++ code via JNI.");
    }
}
