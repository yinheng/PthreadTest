package com.example.haoguo.pthreadtest;

import android.app.Activity;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.example.haoguo.pthreadtest.xhook.XHook;

public class MainActivity extends Activity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("xhookapply");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        //load xhook
        XHook.getInstance().init(this.getApplicationContext());
        if (!XHook.getInstance().isInited()) {
            return;
        }

        XHookApply.init();
        XHookApply.start();

        //xhook do refresh
        XHook.getInstance().refresh(false);

        try {
            Thread.sleep(200);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        //xhook do refresh again
        XHook.getInstance().refresh(false);

        //xhook do refresh again for some reason,
        //maybe called after some System.loadLibrary() and System.load()
        //*
        new Thread(new Runnable() {
            @Override
            public void run() {
                while (true) {
                    XHook.getInstance().refresh(true);

                    try {
                        Thread.sleep(5000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            }
        }).start();

        new Thread(new Runnable() {
            @Override
            public void run() {
                Log.d("DEMO", "I am in new thread");
            }
        }).start();


        setContentView(R.layout.main);
        findViewById(R.id.button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        Log.d("DEMO", "On click...");
                    }
                }).start();

                AsyncTask.THREAD_POOL_EXECUTOR.execute(new Runnable() {
                    @Override
                    public void run() {
                        Log.d("DEMO", "On click...Async task...");
                    }
                });
            }
        });
    }

}
