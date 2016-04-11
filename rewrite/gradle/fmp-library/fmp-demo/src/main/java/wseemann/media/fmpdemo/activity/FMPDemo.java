/*
 * FFmpegMediaPlayer: A unified interface for playing audio files and streams.
 *
 * Copyright 2014 William Seemann
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package wseemann.media.fmpdemo.activity;

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;

import wseemann.media.fmpdemo.service.IMediaPlaybackService;
import wseemann.media.fmpdemo.service.MusicUtils;
import wseemann.media.fmpdemo.service.MusicUtils.ServiceToken;
import wseemann.media.fmpdemo.R;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.Uri;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.support.v4.app.FragmentActivity;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;

public class FMPDemo extends FragmentActivity implements ServiceConnection {

    private IMediaPlaybackService mService = null;
	private ServiceToken mToken;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_fmpdemo);
		
		final EditText uriText = (EditText) findViewById(R.id.uri);
    	// Uncomment for debugging
    	uriText.setText("http://98.212.85.121:53042/Meiko/The Bright Side/01-01- Stuck On You.mp3");
    	
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
					FMPDemo.this.getSystemService(
					Context.INPUT_METHOD_SERVICE);
					imm.hideSoftInputFromWindow(uriText.getWindowToken(), 0);
				
				String uri = uriText.getText().toString();
				
				if (uri.equals("")) {
					uriText.setError(getString(R.string.uri_error));
					return;
				}
				
				String uriString = uriText.getText().toString();
				
				try {
					long [] list = new long[1];
					list[0] = MusicUtils.insert(FMPDemo.this, uriString);
					
					mService.open(list, 0);
				} catch (RemoteException e) {
					e.printStackTrace();
				}
			}
		});
    	
        mToken = MusicUtils.bindToService(this, this);
	}
	
	@Override
	public void onDestroy() {
		MusicUtils.unbindFromService(mToken);
		mService = null;
		
		super.onDestroy();
	}

	@Override
	public void onServiceConnected(ComponentName name, IBinder service) {
		mService = IMediaPlaybackService.Stub.asInterface(service);
	}

	@Override
	public void onServiceDisconnected(ComponentName name) {
		finish();
	}
}
