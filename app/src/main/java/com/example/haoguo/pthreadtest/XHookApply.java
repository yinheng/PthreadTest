package com.example.haoguo.pthreadtest;

public class XHookApply {

    public static synchronized void init() {
        System.loadLibrary("xhookapply");
    }

    public static native void start();
}
