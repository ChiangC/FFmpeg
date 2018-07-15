package com.fmtech.fmplayer;

import android.text.TextUtils;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * ==================================================================
 * Copyright (C) 2018 FMTech All Rights Reserved.
 *
 * @author Drew.Chiang
 * @email chiangchuna@gmail.com
 * @create_date 2018/7/15 14:53
 * <p>
 * ==================================================================
 */

public class FMPlayer implements SurfaceHolder.Callback{
    static{
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

    private SurfaceView mSurfaceView;

    public FMPlayer(SurfaceView surfaceView){
        mSurfaceView = surfaceView;

        surfaceView.getHolder().addCallback(this);
        display(surfaceView.getHolder().getSurface());
    }

    public void playVideo(String path) {
        if (mSurfaceView == null || TextUtils.isEmpty(path)) {
            return;
        }

        play(path);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int format, int width, int height) {
        display(surfaceHolder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    native void play(String path);

    native void display(Surface surface);

    public native void release();

}
