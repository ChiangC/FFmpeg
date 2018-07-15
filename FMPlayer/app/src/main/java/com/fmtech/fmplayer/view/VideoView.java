package com.fmtech.fmplayer.view;

import android.content.Context;
import android.graphics.PixelFormat;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import org.w3c.dom.Text;

/**
 * ==================================================================
 * Copyright (C) 2018 FMTech All Rights Reserved.
 *
 * @author Drew.Chiang
 * @version v1.0.0
 * @email chiangchuna@gmail.com
 * @create_date 2018/7/3 21:49
 * <p>
 * ==================================================================
 */

public class VideoView extends SurfaceView {

    public VideoView(Context context) {
        this(context, null);

    }

    public VideoView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public VideoView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    private void init(){
        SurfaceHolder holder = getHolder();
        holder.setFormat(PixelFormat.RGBA_8888);
    }

   /* public void playVideo(final String url){
        if(!TextUtils.isEmpty(url)){
            new Thread(new Runnable() {
                @Override
                public void run() {
                    render(url, VideoView.this.getHolder().getSurface());
                }
            }).start();
        }
    }*/

//    public native void render(String videoUrl, Surface surface);

}
