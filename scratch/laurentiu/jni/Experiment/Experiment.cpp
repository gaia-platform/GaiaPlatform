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

    const char* name_characters = env->GetStringUTFChars(name, NULL);
    if (name_characters == NULL)
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
 
    std::string full_name = title + name_characters;

    // We need to call this after we're done using name_characters.
    env->ReleaseStringUTFChars(name, name_characters);

    return env->NewStringUTF(full_name.c_str());
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

    jint* int_array = env->GetIntArrayElements(values, NULL);
    if (int_array == NULL)
    {
        return NULL;
    }
    jsize count_values = env->GetArrayLength(values);

    long sum = 0;
    for (int i = 0; i < count_values; i++)
    {
        sum += int_array[i];
    }
    double average = ((double)sum) / count_values;

    // We need to call this after we're done using int_array.
    env->ReleaseIntArrayElements(values, int_array, 0);

    jdouble results[] = { (double)sum, average };

    // Copy results into an array that we can return.
    jdoubleArray output_results = env->NewDoubleArray(2);
    env->SetDoubleArrayRegion(output_results, 0 , 2, results);

    return output_results;
}

JNIEXPORT jobject JNICALL Java_Experiment_getDataContainer(
    JNIEnv* env, jobject, jstring name, jdouble value)
{
    std::cout << "C++ getDataContainer(): Received values: " << name << " and " << value << "." << std::endl;

    jclass data_container_class = env->FindClass("DataContainer");
    jobject new_data_container = env->AllocObject(data_container_class);

    jfieldID name_field = env->GetFieldID(data_container_class , "name", "Ljava/lang/String;");
    jfieldID value_field = env->GetFieldID(data_container_class , "value", "D");

    env->SetObjectField(new_data_container, name_field, name);
    env->SetDoubleField(new_data_container, value_field, value);

    return new_data_container;
}
 
JNIEXPORT jstring JNICALL Java_Experiment_getContainerData(
    JNIEnv* env, jobject, jobject dataContainer)
{
    std::cout << "C++ getContainerData(): Received value: " << dataContainer << "." << std::endl;

    jclass data_container_class = env->GetObjectClass(dataContainer);
    jmethodID method_id = env->GetMethodID(data_container_class, "getData", "()Ljava/lang/String;");

    jstring result = (jstring)env->CallObjectMethod(dataContainer, method_id);

    return result;
}

JNIEXPORT void JNICALL Java_Experiment_throwException(
    JNIEnv* env, jobject)
{
    std::cout << "C++ throwException()." << std::endl;

    jclass exception_class = env->FindClass("java/lang/IllegalArgumentException");
    if (exception_class != NULL)
    {
        env->ThrowNew(exception_class, "This exception was thrown from C++ code via JNI.");
    }
}
