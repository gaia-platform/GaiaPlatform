/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

public class Experiment
{
    static
    {
        System.loadLibrary("native_experiment");
    }
     
    public static void main(String[] args)
    {
        Experiment experiment = new Experiment();

        String alice = experiment.formalizeName("Alice", true);
        sayHelloToMyLittleFriend(alice);

        String bob = experiment.formalizeName("Bob", false);
        sayHelloToMyLittleFriend(bob);

        long sum = experiment.addIntegers(2, 40);
        System.out.println("The sum result is: " + sum + ".\n");

        int countValues = 10;
        int[] values = new int[countValues];
        for (int i = 0; i < countValues; i++)
        {
            values[i] = i + 1;
        }
        double[] results = experiment.addAndAverageIntegers(values);
        System.out.println("The sum result is: " + results[0] + ".");
        System.out.println("The average result is: " + results[1] + ".\n");

        DataContainer dataContainer = experiment.getDataContainer("Pi", 3.1415);
        System.out.println("");

        String data = experiment.getContainerData(dataContainer);
        System.out.println(data);

        try
        {
            experiment.throwException();
        }
        catch (Exception e)
        {
            System.out.println("Java caught an exception:\n\t" + e);
        }

        System.out.println("\nAll experiments were completed!");
    }
 
    private static void sayHelloToMyLittleFriend(String name)
    {
        System.out.println("Hello " + name + "!\n");
    }

    // The native methods that we call via JNI.
    //
    private native String formalizeName(String name, boolean isFemale);

    private native long addIntegers(int first, int second);

    private native double[] addAndAverageIntegers(int[] values);

    private native DataContainer getDataContainer(String name, double value);

    private native String getContainerData(DataContainer dataContainer);

    private native void throwException();
}
