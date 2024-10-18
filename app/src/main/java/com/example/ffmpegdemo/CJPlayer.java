package com.example.ffmpegdemo;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceView;
import android.widget.FrameLayout;

public class CJPlayer implements AudioTrack.OnPlaybackPositionUpdateListener {
    private SurfaceView surfaceView;
    private AudioTrack audioTrack;

    public CJPlayer(SurfaceView surfaceView) {
        this.surfaceView = surfaceView;
    }

    public native void play(String url, Surface surface);


    public native String ffmpegInfo();

    public void onSizeChange(int width, int height) {
        float aspect = width * 1f / height;
        int videoWidth = surfaceView.getContext().getResources().getDisplayMetrics().widthPixels;
        int videoHeight = (int) (videoWidth / aspect);
        FrameLayout.LayoutParams layoutParams = new FrameLayout.LayoutParams(videoWidth, videoHeight);
        surfaceView.setLayoutParams(layoutParams);
    }

    //通道数  采样位数 评率
    public void createTrack(int snampleRateHz, int nb_channals) {
        Log.d("createTrack","createTrack snampleRateHz:"+snampleRateHz + "nb_channals：" +nb_channals);
        int channalConfig;
        if (nb_channals == 1) {
            channalConfig = AudioFormat.CHANNEL_IN_MONO;
        } else if (nb_channals == 2) {
            channalConfig = AudioFormat.CHANNEL_OUT_STEREO;
        } else {
            channalConfig = AudioFormat.CHANNEL_OUT_MONO;
        }
        int bufferSize = AudioTrack.getMinBufferSize(snampleRateHz,channalConfig,AudioFormat.ENCODING_PCM_16BIT);
        audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC,
                snampleRateHz,
                channalConfig,
                AudioFormat.ENCODING_PCM_16BIT,
                bufferSize,
                AudioTrack.MODE_STREAM);
        audioTrack.setPlaybackPositionUpdateListener(this);
        audioTrack.play();
    }

    public void playTrack(byte[] buffer,int length) {
        audioTrack.write(buffer,0,length);
    }


    @Override
    public void onMarkerReached(AudioTrack track) {

    }

    @Override
    public void onPeriodicNotification(AudioTrack track) {

    }
}
