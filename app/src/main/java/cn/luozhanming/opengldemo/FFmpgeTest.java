package cn.luozhanming.opengldemo;

public class FFmpgeTest {

    static {
        System.loadLibrary("ijkffmpeg");
        System.loadLibrary("test-lib");
    }

    public native String jniString();

    public native int decode(String input,String output);

}
