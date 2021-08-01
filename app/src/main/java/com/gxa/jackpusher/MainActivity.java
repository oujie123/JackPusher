package com.gxa.jackpusher;

import androidx.appcompat.app.AppCompatActivity;

import android.hardware.Camera;
import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    private JackPusher mJackPusher;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        TextView tv = findViewById(R.id.sample_text);
        tv.setText(stringFromJNI());

        SurfaceView surfaceView = findViewById(R.id.surfaceView);

        // 前置摄像头，宽，高，fps(每秒25帧)，码率/比特率：https://blog.51cto.com/u_7335580/2058648
        mJackPusher = new JackPusher(this, Camera.CameraInfo.CAMERA_FACING_FRONT, 640, 480, 25, 800000);
        mJackPusher.setPreviewDisplay(surfaceView.getHolder());
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    /**
     * 切换摄像头
     * @param view
     */
    public void switchCamera(View view) {
        mJackPusher.switchCamera();
    }

    /**
     * 开始直播
     * @param view
     */
    public void startLive(View view) {
        mJackPusher.startLive("rtmp://139.224.136.101/myapp");

        //mJackPusher.startLive("rtmp://sendtc3a.douyu.com/live/9835435rvBddb3av?wsSecret=71a52cc241fe730a2e10d27f1c9c0899&wsTime=60c2205f&wsSeek=off&wm=0&tw=0&roirecognition=0&record=flv&origin=tct");

    }

    /**
     * 停止直播
     * @param view
     */
    public void stopLive(View view) {
        mJackPusher.stopLive();
    }

    /**
     * 释放工作
     */
    @Override
    protected void onDestroy() {
        super.onDestroy();
        mJackPusher.release();
    }
}
