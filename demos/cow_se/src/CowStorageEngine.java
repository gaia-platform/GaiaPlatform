/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

public class CowStorageEngine
{
    static
    {
        System.loadLibrary("native_cow_se");
    }
     
    public static void main(String[] args)
    {
        new CowStorageEngine().helloCow();
    }
 
    private native void helloCow();
}
