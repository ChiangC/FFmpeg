package com.fmtech.fmplayer;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Environment;
import android.support.annotation.NonNull;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.WindowManager;

import com.fmtech.fmplayer.view.VideoView;

public class MainActivity extends AppCompatActivity {

    private VideoView mVideoView;
    private FMPlayer mFMPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);

        setContentView(R.layout.activity_main);

        mVideoView = (VideoView) findViewById(R.id.video_view);
        mFMPlayer = new FMPlayer(mVideoView);


        checkPermission();
    }

    private void checkPermission(){
        if(ContextCompat.checkSelfPermission(MainActivity.this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED
                || ContextCompat.checkSelfPermission(MainActivity.this, Manifest.permission.MODIFY_AUDIO_SETTINGS) != PackageManager.PERMISSION_GRANTED){
            requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.MODIFY_AUDIO_SETTINGS}, 100);
        }else{
            playVideo();
        }
    }

    private void playVideo(){
        mVideoView.postDelayed(new Runnable() {
            @Override
            public void run() {
                mFMPlayer.playVideo(Environment.getExternalStorageDirectory().getAbsolutePath() +"/self_driving.mp4");
//                mFMPlayer.playVideo("rtmp://live.hkstv.hk.lxdns.com/live/hks");
            }
        }, 1000);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if(grantResults[0] == PackageManager.PERMISSION_GRANTED){
            playVideo();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mFMPlayer.release();
    }
}
