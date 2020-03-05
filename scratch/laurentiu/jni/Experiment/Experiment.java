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
        System.out.println("The answer is: " + sum);

        DataContainer dataContainer = experiment.getDataContainer("Pi", 3.1415);
        String data = experiment.getContainerData(dataContainer);
        System.out.println(data);
    }
 
    private static void sayHelloToMyLittleFriend(String name)
    {
        System.out.println("Hello " + name + "!");
    }

    private native long addIntegers(int first, int second);
     
    private native String formalizeName(String name, boolean isFemale);

    private native DataContainer getDataContainer(String name, double value);
     
    private native String getContainerData(DataContainer dataContainer);
}
