package com.fmtech.ffmpegdecoder;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Environment;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("avcodec-56");
        System.loadLibrary("avdevice-56");
        System.loadLibrary("avfilter-5");
        System.loadLibrary("avformat-56");
        System.loadLibrary("avutil-54");
        System.loadLibrary("postproc-53");
        System.loadLibrary("swresample-1");
        System.loadLibrary("swscale-3");
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        checkPermission();

        TextView decodeBtn = findViewById(R.id.btn_decode);
        decodeBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String inputPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/self_driving.mp4";
                String outputPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/self_driving.yuv";
                mp4ToYuv(inputPath, outputPath);
            }
        });

        String hello = "hello";
        String he ="he" + new String("llo");
        System.out.println("-------"+(hello == he));
    }

    private void checkPermission(){
        if(ContextCompat.checkSelfPermission(MainActivity.this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED){
            requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE}, 100);
        }
    }


    public native void mp4ToYuv(String inputPath, String outPutPath);


}
