#include "HelloWorldJNI.h"

#include <iostream>

JNIEXPORT void JNICALL Java_HelloWorldJNI_sayHello(JNIEnv*, jobject)
{
    std::cout << "Hello from C++ !!!" << std::endl;
}