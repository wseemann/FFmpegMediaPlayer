package wseemann.media.fmpdemo.activity;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.app.Activity;
import android.view.View;
import android.widget.Button;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import wseemann.media.fmpdemo.R;

public class MainActivity extends Activity {

    private static final int MY_PERMISSIONS_REQUEST_EXTERNAL_STORAGE = 100;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        checkPermission();

        Button audioButton = (Button) findViewById(R.id.audio_button);
        audioButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                MainActivity.this.startActivity(new Intent(MainActivity.this, FMPDemo.class));
            }
        });

        Button videoButton = (Button) findViewById(R.id.video_button);
        videoButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                MainActivity.this.startActivity(new Intent(MainActivity.this, VideoPlayerActivity.class));
            }
        });
    }

    private void checkPermission() {
        // Here, thisActivity is the current activity
        if (ContextCompat.checkSelfPermission(this,
                Manifest.permission.READ_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {

            // Should we show an explanation?
            if (ActivityCompat.shouldShowRequestPermissionRationale(this,
                    Manifest.permission.READ_EXTERNAL_STORAGE)) {

                // Show an expanation to the user *asynchronously* -- don't block
                // this thread waiting for the user's response! After the user
                // sees the explanation, try again to request the permission.

            } else {

                // No explanation needed, we can request the permission.

                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.READ_EXTERNAL_STORAGE},
                        MY_PERMISSIONS_REQUEST_EXTERNAL_STORAGE);

                // MY_PERMISSIONS_REQUEST_EXTERNAL_STORAGE is an
                // app-defined int constant. The callback method gets the
                // result of the request.
            }
        }
    }
}
