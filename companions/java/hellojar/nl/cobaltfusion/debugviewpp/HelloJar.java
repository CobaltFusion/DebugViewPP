package nl.cobaltfusion.debugviewpp;

import nl.cobaltfusion.debugviewpp.DebugViewPP;

public class HelloJar {

    public static void main(String[] args) {
        
        System.out.println("Arguments:");
        DebugViewPP.println("HelloJar from java!");

        for (String arg : args) {
            System.out.println(arg);
            DebugViewPP.println(arg);
        }
    }
}