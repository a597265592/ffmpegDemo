package com.example.ffmpegdemo;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.media.MediaCodec;
import android.os.Bundle;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.widget.TextView;

import com.example.ffmpegdemo.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'ffmpegdemo' library on application startup.
    static {
        System.loadLibrary("ffmpegdemo");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        MediaCodec mediaCodec;
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        binding.surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(@NonNull SurfaceHolder surfaceHolder) {
                play(surfaceHolder.getSurface());
            }

            @Override
            public void surfaceChanged(@NonNull SurfaceHolder surfaceHolder, int i, int i1, int i2) {

            }

            @Override
            public void surfaceDestroyed(@NonNull SurfaceHolder surfaceHolder) {

            }
        });
    }

    private void play(Surface surface) {
        CJPlayer cjPlayer = new CJPlayer(binding.surfaceView);
        cjPlayer.play("http://10.32.99.18:8081/D%3A/share/3_1920x1080_a20180626.mp4",surface);
    }

    /**
     * A native method that is implemented by the 'ffmpegdemo' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}