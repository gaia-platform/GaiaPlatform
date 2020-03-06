/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

public class HelloWorld
{
    static
    {
        System.loadLibrary("native_hello_world");
    }
     
    public static void main(String[] args)
    {
        new HelloWorld().helloWorld();
    }
 
    private native void helloWorld();
}
