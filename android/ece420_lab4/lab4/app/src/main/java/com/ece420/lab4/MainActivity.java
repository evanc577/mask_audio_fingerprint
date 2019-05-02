/*
 * Copyright 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.ece420.lab4;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.Manifest;
import android.graphics.Color;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.MediaMetadataRetriever;
import android.media.MediaPlayer;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.provider.MediaStore;
import android.support.annotation.NonNull;
import android.support.constraint.ConstraintLayout;
import android.support.v4.app.ActivityCompat;
import android.support.v7.widget.AppCompatTextView;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.VideoView;

import java.io.File;
import java.lang.reflect.Field;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.atomic.AtomicBoolean;


public class MainActivity extends Activity
        implements ActivityCompat.OnRequestPermissionsResultCallback {

    // UI Variables
    Button   controlButton;
    static TextView freq_view;
    static TextView start_view;
    String  nativeSampleRate;
    String  nativeSampleBufSize;
    File externalFilesDir;
    String externalFilesDirStr;
    String currentText = "";
    boolean supportRecording;
    AtomicBoolean isPlaying = new AtomicBoolean(false);
    // Static Values
    private static final int AUDIO_ECHO_REQUEST = 0;
    private static final int FRAME_SIZE = 4800;

    private static final int VIDEO_DELAY = 2500;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setContentView(R.layout.activity_main);
        super.setRequestedOrientation (ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

        externalFilesDir = getExternalFilesDir(null);
        externalFilesDirStr = externalFilesDir.getAbsolutePath();
        if (!externalFilesDir.exists()) {
            externalFilesDir.mkdir();
        }

        // Google NDK Stuff
        controlButton = (Button)findViewById((R.id.capture_control_button));
        controlButton.setBackground(getDrawable(R.drawable.start_button));
        queryNativeAudioParameters();
        // initialize native audio system
        updateNativeAudioUI();
        if (supportRecording) {
            createSLEngine(Integer.parseInt(nativeSampleRate), FRAME_SIZE);
        }

        // Setup UI
        freq_view = (TextView)findViewById(R.id.textFrequency);
        freq_view.setVisibility(View.GONE);
        freq_view.setSelected(true);
        start_view = (TextView)findViewById(R.id.textStart);
        start_view.setVisibility(View.VISIBLE);

        initializeMaskTextBackgroundTask(250);
        initializeMaskStatusBackgroundTask(10);

        VideoView vv = findViewById(R.id.videoView);
        vv.setVisibility(View.INVISIBLE);
        vv.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
            @Override
            public void onCompletion(MediaPlayer mp) {
                VideoView vv = findViewById(R.id.videoView);
                vv.setVisibility(View.INVISIBLE);
                vv.stopPlayback();
                vv.suspend();
            }
        });
        vv.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {
            @Override
            public void onPrepared(MediaPlayer mp) {
                int width = mp.getVideoWidth();
                int height = mp.getVideoHeight();
                VideoView vv = findViewById(R.id.videoView);
                ConstraintLayout.LayoutParams params = (ConstraintLayout.LayoutParams)vv.getLayoutParams();
                params.width = vv.getMeasuredWidth();
                params.height = params.width * height / width;
                vv.setLayoutParams(params);
            }
        });
    }
    @Override
    protected synchronized void onDestroy() {
        if (supportRecording) {
            if (isPlaying.get()) {
                stopPlay();
            }
            deleteSLEngine();
            isPlaying.set(false);
        }
        super.onDestroy();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        return false;
    }

    private synchronized void startEcho() {
        if(!supportRecording){
            return;
        }
        if (!isPlaying.get()) {
            if(!createSLBufferQueueAudioPlayer()) {
                return;
            }
            if(!createAudioRecorder()) {
                deleteSLBufferQueueAudioPlayer();
                return;
            }
            initIdentify(externalFilesDirStr);
            startPlay();   // this must include startRecording()

            // update spinner
            ProgressBar spinner;
            spinner = (ProgressBar)findViewById(R.id.progressBar);
            spinner.setIndeterminate(true);

            // update other ui elements
            controlButton.setText(getString(R.string.StopEcho));
            controlButton.setBackground(getDrawable(R.drawable.stop_button));
            start_view.setVisibility(View.GONE);
            freq_view.setVisibility(View.VISIBLE);
            VideoView vv = findViewById(R.id.videoView);
            vv.setVisibility(View.INVISIBLE);
            vv.stopPlayback();
            vv.suspend();
            isPlaying.set(true);
        } else {
            timeout();
        }
    }

    public synchronized void foundSong() {
        if (!isPlaying.get()) {
            return;
        }
        isPlaying.set(false);

        if (Build.VERSION.SDK_INT >= 26) {
            ((Vibrator) getSystemService(VIBRATOR_SERVICE)).vibrate(VibrationEffect.createOneShot(250, VibrationEffect.DEFAULT_AMPLITUDE));
        } else {
            ((Vibrator) getSystemService(VIBRATOR_SERVICE)).vibrate(250);
        }

        // proof of concept video sync
        String song = getMaskText();
        String videoPath = "";
        int time = getMaskTime() + VIDEO_DELAY;
        VideoView vv = findViewById(R.id.videoView);

        if (song.equals("siren_audio.flac")) {
            videoPath = externalFilesDirStr + "/videos/siren_video.mp4";
        } else if (song.equals("fancy_audio.flac")) {
            videoPath = externalFilesDirStr + "/videos/fancy_video.mp4";
        } else if (song.equals("chocolate_audio.flac")) {
            videoPath = externalFilesDirStr + "/videos/chocolate_video.mp4";
        } else if (song.equals("hey_audio.flac")) {
            videoPath = externalFilesDirStr + "/videos/hey_video.mp4";
        }

        if (!videoPath.equals("")) {
            vv.setVideoPath(videoPath);
            vv.seekTo(time);
            vv.start();
            vv.setVisibility(View.VISIBLE);
        }

        // update spinner
        ProgressBar spinner;
        spinner = (ProgressBar)findViewById(R.id.progressBar);
        spinner.setIndeterminate(false);
        spinner.setProgress(100);

        stopPlay();
        deleteIdentify();
        updateNativeAudioUI();
        deleteAudioRecorder();
        deleteSLBufferQueueAudioPlayer();
        controlButton.setText(getString(R.string.StartEcho));
        controlButton.setBackground(getDrawable(R.drawable.start_button));
    }

    public synchronized void timeout() {
        if (!isPlaying.get()) {
            return;
        }
        isPlaying.set(false);

        // update spinner
        ProgressBar spinner;
        spinner = (ProgressBar)findViewById(R.id.progressBar);
        spinner.setIndeterminate(false);
        spinner.setProgress(0);

        stopPlay();
        deleteIdentify();
        updateNativeAudioUI();
        deleteAudioRecorder();
        deleteSLBufferQueueAudioPlayer();
        controlButton.setText(getString(R.string.StartEcho));
        controlButton.setBackground(getDrawable(R.drawable.start_button));
        freq_view.setVisibility(View.GONE);
    start_view.setVisibility(View.VISIBLE);
    }

    public void onEchoClick(View view) {
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) !=
                PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(
                    this,
                    new String[] { Manifest.permission.RECORD_AUDIO },
                    AUDIO_ECHO_REQUEST);
            return;
        }
        startEcho();
    }

    private void queryNativeAudioParameters() {
        AudioManager myAudioMgr = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        nativeSampleRate  =  myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
        nativeSampleBufSize =myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
        int recBufSize = AudioRecord.getMinBufferSize(
                Integer.parseInt(nativeSampleRate),
                AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_16BIT);
        supportRecording = true;
        if (recBufSize == AudioRecord.ERROR ||
                recBufSize == AudioRecord.ERROR_BAD_VALUE) {
            supportRecording = false;
        }
    }
    private void updateNativeAudioUI() {
        if (!supportRecording) {
            controlButton.setEnabled(false);
            return;
        }
    }
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        /*
         * if any permission failed, the sample could not play
         */
        if (AUDIO_ECHO_REQUEST != requestCode) {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
            return;
        }

        if (grantResults.length != 1  ||
                grantResults[0] != PackageManager.PERMISSION_GRANTED) {
            /*
             * When user denied permission, throw a Toast to prompt that RECORD_AUDIO
             * is necessary; also display the status on UI
             * Then application goes back to the original state: it behaves as if the button
             * was not clicked. The assumption is that user will re-click the "start" button
             * (to retry), or shutdown the app in normal way.
             */
            Toast.makeText(getApplicationContext(),
                    getString(R.string.prompt_permission),
                    Toast.LENGTH_SHORT).show();
            return;
        }

        /*
         * When permissions are granted, we prompt the user the status. User would
         * re-try the "start" button to perform the normal operation. This saves us the extra
         * logic in code for async processing of the button listener.
         */

        // The callback runs on app's thread, so we are safe to resume the action
        startEcho();
    }

    // All this does is calls the UpdateStftTask at a fixed interval
    // http://stackoverflow.com/questions/6531950/how-to-execute-async-task-repeatedly-after-fixed-time-intervals
    public void initializeMaskTextBackgroundTask(int timeInMs) {
        final Handler handler = new Handler();
        Timer timer = new Timer();
        TimerTask doAsynchronousTask = new TimerTask() {
            @Override
            public void run() {
                handler.post(new Runnable() {
                    public void run() {
                        try {
                            UpdateMaskTextTask performMaskTextUpdate = new UpdateMaskTextTask();
                            performMaskTextUpdate.execute();
                        } catch (Exception e) {
                            // TODO Auto-generated catch block
                        }
                    }
                });
            }
        };
        timer.schedule(doAsynchronousTask, 0, timeInMs); // execute every 100 ms
    }

    // UI update
    private class UpdateMaskTextTask extends AsyncTask<Void, String, Void> {
        @Override
        protected Void doInBackground(Void... params) {

            // Update screen, needs to be done on UI thread
            publishProgress(getMaskText());

            return null;
        }

        protected void onProgressUpdate(String... newString) {
            if (!currentText.equals(newString[0])) {
                freq_view.setText(newString[0]);
                currentText = newString[0];
            }
        }
    }

    public void initializeMaskStatusBackgroundTask(int timeInMs) {
        final Handler handler = new Handler();
        Timer timer = new Timer();
        TimerTask doAsynchronousTask = new TimerTask() {
            @Override
            public void run() {
                handler.post(new Runnable() {
                    public void run() {
                        try {
                            UpdateMaskStatusTask performMaskStatusUpdate = new UpdateMaskStatusTask();
                            performMaskStatusUpdate.execute();
                        } catch (Exception e) {
                            // TODO Auto-generated catch block
                        }
                    }
                });
            }
        };
        timer.schedule(doAsynchronousTask, 0, timeInMs); // execute every 100 ms
    }

    // UI update
    private class UpdateMaskStatusTask extends AsyncTask<Void, Integer, Void> {
        @Override
        protected Void doInBackground(Void... params) {

            // Update screen, needs to be done on UI thread
            publishProgress(getMaskStatus());

            return null;
        }

        protected void onProgressUpdate(Integer... v) {
            if (v[0] == 0) {
                return;
            } else if (v[0] == 1) { // found
                foundSong();
            } else if (v[0] == 2) { // timeout
                timeout();
            }
        }
    }
    /*
     * Loading our Libs
     */
    static {
        System.loadLibrary("echo");
    }

    /*
     * jni function implementations...
     */
    public static native void createSLEngine(int rate, int framesPerBuf);
    public static native void deleteSLEngine();

    public static native boolean createSLBufferQueueAudioPlayer();
    public static native void deleteSLBufferQueueAudioPlayer();

    public static native boolean createAudioRecorder();
    public static native void deleteAudioRecorder();
    public static native void startPlay();
    public static native void stopPlay();
    public static native void initIdentify(String s);
    public static native void deleteIdentify();

    public native String getMaskText();
    public native int getMaskStatus();
    public native int getMaskTime();
}
