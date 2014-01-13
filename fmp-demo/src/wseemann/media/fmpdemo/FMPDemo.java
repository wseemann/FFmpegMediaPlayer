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

package wseemann.media.fmpdemo;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;

import wseemann.media.FFmpegMediaPlayer;
import wseemann.media.fmpdemo.R;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;

public class FMPDemo extends FragmentActivity {

	private FFmpegMediaPlayer mPlayer;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity);
		
		mPlayer = new FFmpegMediaPlayer();
		
		final EditText uriText = (EditText) findViewById(R.id.uri);
    	// Uncomment for debugging
    	//uriText.setText("http://...");
    	
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
				
				mPlayer.reset();
				mPlayer.setOnPreparedListener(new FFmpegMediaPlayer.OnPreparedListener() {

				    @Override
				    public void onPrepared(FFmpegMediaPlayer mp) {
				        mp.start();
				    }
				});
				
				mPlayer.setOnCompletionListener(new FFmpegMediaPlayer.OnCompletionListener() {
					
					@Override
					public void onCompletion(FFmpegMediaPlayer mp) {
						
					}
				});
				
				mPlayer.setOnErrorListener(new FFmpegMediaPlayer.OnErrorListener() {

				    @Override
				    public boolean onError(FFmpegMediaPlayer mp, int what, int extra) {
				    	
				        return true;
				    }
				   
				});
				
				try {
					mPlayer.setDataSource(uriString);
					mPlayer.prepareAsync();
				} catch (IllegalArgumentException e) {
					e.printStackTrace();
				} catch (SecurityException e) {
					e.printStackTrace();
				} catch (IllegalStateException e) {
					e.printStackTrace();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
		});
    	
	}
	
	@Override
	public void onStop() {
		super.onStop();
		
		mPlayer.reset();
		mPlayer.release();
	}
}
