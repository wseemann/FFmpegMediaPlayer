package wseemann.media.fmpdemo.activity;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;

import androidx.fragment.app.FragmentActivity;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;

import wseemann.media.FFmpegMediaPlayer;
import wseemann.media.fmpdemo.R;

/**
 * Created by wseemann on 4/10/16.
 */
public class VideoPlayerActivity extends FragmentActivity {

    private SurfaceView mmSurfaceView;
    private SurfaceHolder mSurfaceHolder;
    private Surface mFinalSurface;

    private FFmpegMediaPlayer mMediaPlayer;

    public void onCreate(Bundle icicle) {
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        super.onCreate(icicle);
        setContentView(R.layout.activity_video_player);

        final EditText uriText = (EditText) findViewById(R.id.uri);
        // Uncomment for debugging
        uriText.setText("https://test-videos.co.uk/vids/bigbuckbunny/mp4/h264/1080/Big_Buck_Bunny_1080_10s_1MB.mp4");

        Intent intent = getIntent();

        // Populate the edit text field with the intent uri, if available
        Uri uri = intent.getData();

        if (intent.getExtras() != null &&
                intent.getExtras().getCharSequence(Intent.EXTRA_TEXT) != null) {
            uri = Uri.parse(intent.getExtras().getCharSequence(Intent.EXTRA_TEXT).toString());
        }

        if (uri != null) {
            try {
                uriText.setText(URLDecoder.decode(uri.toString(), "UTF-8"));
            } catch (UnsupportedEncodingException e1) {
            }
        }

        setIntent(null);

        Button goButton = (Button) findViewById(R.id.go_button);
        goButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                // Clear the error message
                uriText.setError(null);

                // Hide the keyboard
                InputMethodManager imm = (InputMethodManager)
                        getSystemService(
                                Context.INPUT_METHOD_SERVICE);
                imm.hideSoftInputFromWindow(uriText.getWindowToken(), 0);

                String uri = uriText.getText().toString();

                if (uri.equals("")) {
                    uriText.setError(getString(R.string.uri_error));
                    return;
                }

                String uriString = uriText.getText().toString();

                try {
                    mMediaPlayer.reset();

                    mMediaPlayer.setDataSource(uriString);
                    if (mFinalSurface != null) {
                       mMediaPlayer.setSurface(mFinalSurface);
                    }
                    mMediaPlayer.prepareAsync();
                } catch (IOException ex) {
                    ex.printStackTrace();
                }
            }
        });

        // set up the Surface video sink
        mmSurfaceView = (SurfaceView) findViewById(R.id.surfaceview);
        mSurfaceHolder = mmSurfaceView.getHolder();

        mSurfaceHolder.addCallback(new SurfaceHolder.Callback() {

            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                Log.v("TAG", "surfaceChanged format=" + format + ", width=" + width + ", height="
                        + height);
            }

            public void surfaceCreated(SurfaceHolder holder) {
                mFinalSurface = holder.getSurface();

                //mMediaPlayer.setSurface(mFinalSurface);

                final EditText uriText = (EditText) findViewById(R.id.uri);

                // Clear the error message
                uriText.setError(null);

                // Hide the keyboard
                InputMethodManager imm = (InputMethodManager)
                        getSystemService(
                                Context.INPUT_METHOD_SERVICE);
                imm.hideSoftInputFromWindow(uriText.getWindowToken(), 0);

                String uri = uriText.getText().toString();

                if (uri.equals("")) {
                    uriText.setError(getString(R.string.uri_error));
                    return;
                }

                String uriString = uriText.getText().toString();
            }

            public void surfaceDestroyed(SurfaceHolder holder) {
                Log.v("TAG", "surfaceDestroyed");
            }

        });

        mMediaPlayer = new FFmpegMediaPlayer();
        mMediaPlayer.setOnPreparedListener(mOnPreparedListener);
        mMediaPlayer.setOnErrorListener(mOnErrorListener);
        mMediaPlayer.stop();
    }

    private FFmpegMediaPlayer.OnPreparedListener mOnPreparedListener = mp -> mp.start();

    private FFmpegMediaPlayer.OnErrorListener mOnErrorListener = (mp, what, extra) -> {
        Log.d(VideoPlayerActivity.class.getName(), "Error: " + what);
        return true;
    };

    @Override
    public void onDestroy() {
        super.onDestroy();

        if (mMediaPlayer != null) {
            mMediaPlayer.stop();
            mMediaPlayer.release();
        }
    }
}
