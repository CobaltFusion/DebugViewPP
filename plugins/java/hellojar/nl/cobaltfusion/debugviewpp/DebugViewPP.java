package nl.cobaltfusion.debugviewpp;

import com.sun.jna.Library;
import com.sun.jna.Native;

public class DebugViewPP
{
    public interface Kernel32 extends Library
    {
        public void OutputDebugStringA(String Text);
    }
    
    /**
    * This method makes a Kernel32 Call and sends the debugstring to the
    * KernelConsole.
    */
    static void println(String message)
    {
        Kernel32 lib = (Kernel32) Native.loadLibrary("kernel32", Kernel32.class);
        lib.OutputDebugStringA(message);
    }
}
