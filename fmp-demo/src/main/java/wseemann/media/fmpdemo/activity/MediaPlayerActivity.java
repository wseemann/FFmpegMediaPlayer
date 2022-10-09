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

import wseemann.media.fmpdemo.R;
import wseemann.media.fmpdemo.fragment.MediaPlayerFragment;
import wseemann.media.fmpdemo.service.IMediaPlaybackService;
import wseemann.media.fmpdemo.service.MediaPlaybackService;
import wseemann.media.fmpdemo.service.MusicUtils;
import wseemann.media.fmpdemo.service.MusicUtils.ServiceToken;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.res.Configuration;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Parcelable;
import android.os.RemoteException;
import android.os.SystemClock;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.SeekBar.OnSeekBarChangeListener;

import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentStatePagerAdapter;
import androidx.viewpager.widget.ViewPager;

public class MediaPlayerActivity extends FragmentActivity {
	
    private static final String TAG = MediaPlayerActivity.class.getName();

	private DialogFragment mLoadingDialog;
	
    private static int VISIBLE = 1;
    private static int GONE = 2;
    
    private boolean mSeeking = false;
    private boolean mDeviceHasDpad;
    private long mStartSeekPos = 0;
    private long mLastSeekEventTime;
    private IMediaPlaybackService mService = null;
    private ImageButton mPrevButton;
    private ImageButton mPauseButton;
    private ImageButton mNextButton;
    private ImageButton mRepeatButton;
    private ImageButton mShuffleButton;
    private Toast mToast;
    private ServiceToken mToken;

	private ViewPager mPager;
	private GridPagerAdapter mAdapter;
    
    public MediaPlayerActivity()
    {
    }

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle icicle) {
    	getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        super.onCreate(icicle);
        setContentView(R.layout.activity_media_player);
        
        setVolumeControlStream(AudioManager.STREAM_MUSIC);

        mCurrentTime = (TextView) findViewById(R.id.position_text);
        mTotalTime = (TextView) findViewById(R.id.duration_text);
        mProgress = (ProgressBar) findViewById(R.id.seek_bar);

        mPrevButton = (ImageButton) findViewById(R.id.previous_button);
        mPrevButton.setOnClickListener(mPrevListener);
        mPauseButton = (ImageButton) findViewById(R.id.play_pause_button);
        mPauseButton.setOnClickListener(mPauseListener);
        mNextButton = (ImageButton) findViewById(R.id.next_button);
        mNextButton.setOnClickListener(mNextListener);
        seekmethod = 1;

        mDeviceHasDpad = (getResources().getConfiguration().navigation ==
            Configuration.NAVIGATION_DPAD);
        
        mShuffleButton = (ImageButton) findViewById(R.id.shuffle_button);
        mShuffleButton.setOnClickListener(mShuffleListener);        
        mRepeatButton = (ImageButton) findViewById(R.id.repeat_button);
        mRepeatButton.setOnClickListener(mRepeatListener);
        
        if (mProgress instanceof SeekBar) {
            SeekBar seeker = (SeekBar) mProgress;
            seeker.setOnSeekBarChangeListener(mSeekListener);
        }
        mProgress.setMax(1000);
        
        mPager = (ViewPager) findViewById(R.id.pager);
    }
    
    private OnSeekBarChangeListener mSeekListener = new OnSeekBarChangeListener() {
        public void onStartTrackingTouch(SeekBar bar) {
            mLastSeekEventTime = 0;
            mFromTouch = true;
        }
        public void onProgressChanged(SeekBar bar, int progress, boolean fromuser) {
            if (!fromuser || (mService == null)) return;
            long now = SystemClock.elapsedRealtime();
            if ((now - mLastSeekEventTime) > 250) {
                mLastSeekEventTime = now;
                mPosOverride = mDuration * progress / 1000;
                try {
                    mService.seek(mPosOverride);
                } catch (RemoteException ex) {
                }

                // trackball event, allow progress updates
                if (!mFromTouch) {
                    refreshNow();
                    mPosOverride = -1;
                }
            }
        }
        public void onStopTrackingTouch(SeekBar bar) {
            mPosOverride = -1;
            mFromTouch = false;
        }
    };

    private View.OnClickListener mShuffleListener = new View.OnClickListener() {
        public void onClick(View v) {
            toggleShuffle();
        }
    };

    private View.OnClickListener mRepeatListener = new View.OnClickListener() {
        public void onClick(View v) {
            cycleRepeat();
        }
    };

    private View.OnClickListener mPauseListener = new View.OnClickListener() {
        public void onClick(View v) {
            doPauseResume();
        }
    };
    
    private View.OnClickListener mPrevListener = new View.OnClickListener() {
        public void onClick(View v) {
            if (mService == null) return;
            try {
                mService.prev();
            } catch (RemoteException ex) {
            }
        }
    };

    private View.OnClickListener mNextListener = new View.OnClickListener() {
        public void onClick(View v) {
            if (mService == null) return;
            try {
                mService.next();
            } catch (RemoteException ex) {
            }
        }
    };

    @Override
    public void onStart() {
        super.onStart();
        paused = false;
        
        mToken = MusicUtils.bindToService(this, osc);
        if (mToken == null) {
            // something went wrong
            mHandler.sendEmptyMessage(QUIT);
        }
        
        IntentFilter f = new IntentFilter();
        f.addAction(MediaPlaybackService.PLAYSTATE_CHANGED);
        f.addAction(MediaPlaybackService.META_CHANGED);
        //f.addAction(MediaPlaybackService.START_DIALOG);
        //f.addAction(MediaPlaybackService.STOP_DIALOG);
        registerReceiver(mStatusListener, new IntentFilter(f));
        updateTrackInfo();
        long next = refreshNow();
        queueNextRefresh(next);
    }
    
    @Override
    public void onResume() {
        super.onResume();
        updateTrackInfo();
        setPauseButtonImage();
    }

    @Override
    public void onPause() {
    	super.onPause();
    	dismissLoadingDialog();
    }

    @Override
    public void onStop() {
        paused = true;
        mHandler.removeMessages(REFRESH);
        unregisterReceiver(mStatusListener);
        MusicUtils.unbindFromService(mToken);
        mService = null;
        super.onStop();
    }

    private final int keyboard[][] = {
        {
            KeyEvent.KEYCODE_Q,
            KeyEvent.KEYCODE_W,
            KeyEvent.KEYCODE_E,
            KeyEvent.KEYCODE_R,
            KeyEvent.KEYCODE_T,
            KeyEvent.KEYCODE_Y,
            KeyEvent.KEYCODE_U,
            KeyEvent.KEYCODE_I,
            KeyEvent.KEYCODE_O,
            KeyEvent.KEYCODE_P,
        },
        {
            KeyEvent.KEYCODE_A,
            KeyEvent.KEYCODE_S,
            KeyEvent.KEYCODE_D,
            KeyEvent.KEYCODE_F,
            KeyEvent.KEYCODE_G,
            KeyEvent.KEYCODE_H,
            KeyEvent.KEYCODE_J,
            KeyEvent.KEYCODE_K,
            KeyEvent.KEYCODE_L,
            KeyEvent.KEYCODE_DEL,
        },
        {
            KeyEvent.KEYCODE_Z,
            KeyEvent.KEYCODE_X,
            KeyEvent.KEYCODE_C,
            KeyEvent.KEYCODE_V,
            KeyEvent.KEYCODE_B,
            KeyEvent.KEYCODE_N,
            KeyEvent.KEYCODE_M,
            KeyEvent.KEYCODE_COMMA,
            KeyEvent.KEYCODE_PERIOD,
            KeyEvent.KEYCODE_ENTER
        }

    };

    private int lastX;
    private int lastY;

    private boolean seekMethod1(int keyCode)
    {
        if (mService == null) return false;
        for(int x=0;x<10;x++) {
            for(int y=0;y<3;y++) {
                if(keyboard[y][x] == keyCode) {
                    int dir = 0;
                    // top row
                    if(x == lastX && y == lastY) dir = 0;
                    else if (y == 0 && lastY == 0 && x > lastX) dir = 1;
                    else if (y == 0 && lastY == 0 && x < lastX) dir = -1;
                    // bottom row
                    else if (y == 2 && lastY == 2 && x > lastX) dir = -1;
                    else if (y == 2 && lastY == 2 && x < lastX) dir = 1;
                    // moving up
                    else if (y < lastY && x <= 4) dir = 1; 
                    else if (y < lastY && x >= 5) dir = -1; 
                    // moving down
                    else if (y > lastY && x <= 4) dir = -1; 
                    else if (y > lastY && x >= 5) dir = 1; 
                    lastX = x;
                    lastY = y;
                    try {
                        mService.seek(mService.position() + dir * 5);
                    } catch (RemoteException ex) {
                    }
                    refreshNow();
                    return true;
                }
            }
        }
        lastX = -1;
        lastY = -1;
        return false;
    }

    private boolean seekMethod2(int keyCode)
    {
        if (mService == null) return false;
        for(int i=0;i<10;i++) {
            if(keyboard[0][i] == keyCode) {
                int seekpercentage = 100*i/10;
                try {
                    mService.seek(mService.duration() * seekpercentage / 100);
                } catch (RemoteException ex) {
                }
                refreshNow();
                return true;
            }
        }
        return false;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        try {
            switch(keyCode)
            {
                case KeyEvent.KEYCODE_DPAD_LEFT:
                    if (!useDpadMusicControl()) {
                        break;
                    }
                    if (mService != null) {
                        if (!mSeeking && mStartSeekPos >= 0) {
                            mPauseButton.requestFocus();
                            if (mStartSeekPos < 1000) {
                                mService.prev();
                            } else {
                                mService.seek(0);
                            }
                        } else {
                            scanBackward(-1, event.getEventTime() - event.getDownTime());
                            mPauseButton.requestFocus();
                            mStartSeekPos = -1;
                        }
                    }
                    mSeeking = false;
                    mPosOverride = -1;
                    return true;
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    if (!useDpadMusicControl()) {
                        break;
                    }
                    if (mService != null) {
                        if (!mSeeking && mStartSeekPos >= 0) {
                            mPauseButton.requestFocus();
                            mService.next();
                        } else {
                            scanForward(-1, event.getEventTime() - event.getDownTime());
                            mPauseButton.requestFocus();
                            mStartSeekPos = -1;
                        }
                    }
                    mSeeking = false;
                    mPosOverride = -1;
                    return true;
            }
        } catch (RemoteException ex) {
        }
        return super.onKeyUp(keyCode, event);
    }

    private boolean useDpadMusicControl() {
        if (mDeviceHasDpad && (mPrevButton.isFocused() ||
                mNextButton.isFocused() ||
                mPauseButton.isFocused())) {
            return true;
        }
        return false;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        int repcnt = event.getRepeatCount();

        if((seekmethod==0)?seekMethod1(keyCode):seekMethod2(keyCode))
            return true;

        switch(keyCode)
        {
            case KeyEvent.KEYCODE_SLASH:
                seekmethod = 1 - seekmethod;
                return true;

            case KeyEvent.KEYCODE_DPAD_LEFT:
                if (!useDpadMusicControl()) {
                    break;
                }
                if (!mPrevButton.hasFocus()) {
                    mPrevButton.requestFocus();
                }
                scanBackward(repcnt, event.getEventTime() - event.getDownTime());
                return true;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                if (!useDpadMusicControl()) {
                    break;
                }
                if (!mNextButton.hasFocus()) {
                    mNextButton.requestFocus();
                }
                scanForward(repcnt, event.getEventTime() - event.getDownTime());
                return true;

            case KeyEvent.KEYCODE_S:
                toggleShuffle();
                return true;

            case KeyEvent.KEYCODE_DPAD_CENTER:
            case KeyEvent.KEYCODE_SPACE:
                doPauseResume();
                return true;
        }
        return super.onKeyDown(keyCode, event);
    }
    
    private void scanBackward(int repcnt, long delta) {
        if(mService == null) return;
        try {
            if(repcnt == 0) {
                mStartSeekPos = mService.position();
                mLastSeekEventTime = 0;
                mSeeking = false;
            } else {
                mSeeking = true;
                if (delta < 5000) {
                    // seek at 10x speed for the first 5 seconds
                    delta = delta * 10; 
                } else {
                    // seek at 40x after that
                    delta = 50000 + (delta - 5000) * 40;
                }
                long newpos = mStartSeekPos - delta;
                if (newpos < 0) {
                    // move to previous track
                    mService.prev();
                    long duration = mService.duration();
                    mStartSeekPos += duration;
                    newpos += duration;
                }
                if (((delta - mLastSeekEventTime) > 250) || repcnt < 0){
                    mService.seek(newpos);
                    mLastSeekEventTime = delta;
                }
                if (repcnt >= 0) {
                    mPosOverride = newpos;
                } else {
                    mPosOverride = -1;
                }
                refreshNow();
            }
        } catch (RemoteException ex) {
        }
    }

    private void scanForward(int repcnt, long delta) {
        if(mService == null) return;
        try {
            if(repcnt == 0) {
                mStartSeekPos = mService.position();
                mLastSeekEventTime = 0;
                mSeeking = false;
            } else {
                mSeeking = true;
                if (delta < 5000) {
                    // seek at 10x speed for the first 5 seconds
                    delta = delta * 10; 
                } else {
                    // seek at 40x after that
                    delta = 50000 + (delta - 5000) * 40;
                }
                long newpos = mStartSeekPos + delta;
                long duration = mService.duration();
                if (newpos >= duration) {
                    // move to next track
                    mService.next();
                    mStartSeekPos -= duration; // is OK to go negative
                    newpos -= duration;
                }
                if (((delta - mLastSeekEventTime) > 250) || repcnt < 0){
                    mService.seek(newpos);
                    mLastSeekEventTime = delta;
                }
                if (repcnt >= 0) {
                    mPosOverride = newpos;
                } else {
                    mPosOverride = -1;
                }
                refreshNow();
            }
        } catch (RemoteException ex) {
        }
    }
    
    private void doPauseResume() {
        try {
            if(mService != null) {
                if (mService.isPlaying()) {
                    mService.pause();
                } else {
                    mService.play();
                }
                refreshNow();
                setPauseButtonImage();
            }
        } catch (RemoteException ex) {
        }
    }
    
    private void toggleShuffle() {
        if (mService == null) {
            return;
        }
        try {
            int shuffle = mService.getShuffleMode();
            if (shuffle == MediaPlaybackService.SHUFFLE_NONE) {
            	mService.setShuffleMode(MediaPlaybackService.SHUFFLE_NORMAL);
                if (mService.getRepeatMode() == MediaPlaybackService.REPEAT_CURRENT) {
                	mService.setRepeatMode(MediaPlaybackService.REPEAT_ALL);
                    setRepeatButtonImage();
                }
                showToast(R.string.shuffle_on_notif);
            } else if (shuffle == MediaPlaybackService.SHUFFLE_NORMAL) {
            	mService.setShuffleMode(MediaPlaybackService.SHUFFLE_NONE);
                showToast(R.string.shuffle_off_notif);
            } else {
                Log.e(TAG, "Invalid shuffle mode: " + shuffle);
            }
            setShuffleButtonImage();
        } catch (RemoteException ex) {
        }
    }
    
    private void cycleRepeat() {
        if (mService == null) {
            return;
        }
        try {
            int mode = mService.getRepeatMode();
            if (mode == MediaPlaybackService.REPEAT_NONE) {
                mService.setRepeatMode(MediaPlaybackService.REPEAT_ALL);
                showToast(R.string.repeat_all_notif);
            } else if (mode == MediaPlaybackService.REPEAT_ALL) {
                mService.setRepeatMode(MediaPlaybackService.REPEAT_CURRENT);
                if (mService.getShuffleMode() != MediaPlaybackService.SHUFFLE_NONE) {
                    mService.setShuffleMode(MediaPlaybackService.SHUFFLE_NONE);
                    setShuffleButtonImage();
                }
                showToast(R.string.repeat_current_notif);
            } else {
                mService.setRepeatMode(MediaPlaybackService.REPEAT_NONE);
                showToast(R.string.repeat_off_notif);
            }
            setRepeatButtonImage();
        } catch (RemoteException ex) {
        }
    }
    
    private void showToast(int resid) {
        if (mToast == null) {
            mToast = Toast.makeText(this, null, Toast.LENGTH_SHORT);
        }
        mToast.setText(resid);
        mToast.show();
    }
    
    private void startPlayback() {

        if(mService == null)
            return;

        updateTrackInfo();
        long next = refreshNow();
        queueNextRefresh(next);
    }

    private ServiceConnection osc = new ServiceConnection() {
            public void onServiceConnected(ComponentName classname, IBinder obj) {
                mService = IMediaPlaybackService.Stub.asInterface(obj);
                startPlayback();
                try {
                    // Assume something is playing when the service says it is,
                    // but also if the audio ID is valid but the service is paused.
                    if (mService.getAudioId() >= 0 || mService.isPlaying() ||
                            mService.getPath() != null) {
                        // something is playing now, we're done
                        mRepeatButton.setVisibility(View.VISIBLE);
                        mShuffleButton.setVisibility(View.VISIBLE);
                        setRepeatButtonImage();
                        setShuffleButtonImage();
                        setPauseButtonImage();
                        initPager();
                        return;
                    }
                } catch (RemoteException ex) {
                }
                // Service is dead or not playing anything. Return to the previous
                // activity.
                finish();
            }
            public void onServiceDisconnected(ComponentName classname) {
                mService = null;
            }
    };
    
    private void setRepeatButtonImage() {
        if (mService == null) return;
        try {
            switch (mService.getRepeatMode()) {
                case MediaPlaybackService.REPEAT_ALL:
                    //mRepeatButton.setImageResource(R.drawable.ic_av_repeat_selected);
                    break;
                case MediaPlaybackService.REPEAT_CURRENT:
                    //mRepeatButton.setImageResource(R.drawable.ic_av_repeat_one_selected);
                    break;
                default:
                    //mRepeatButton.setImageResource(Utils.getThemedIcon(this, R.attr.ic_av_repeat));
                    break;
            }
        } catch (RemoteException ex) {
        }
    }
    
    private void setShuffleButtonImage() {
        if (mService == null) return;
        try {
            switch (mService.getShuffleMode()) {
                case MediaPlaybackService.SHUFFLE_NONE:
                    //mShuffleButton.setImageResource(Utils.getThemedIcon(this, R.attr.ic_av_shuffle));
                    break;
                default:
                    //mShuffleButton.setImageResource(R.drawable.ic_av_shuffle_selected);
                    break;
            }
        } catch (RemoteException ex) {
        }
    }
    
    private void setPauseButtonImage() {
        try {
            if (mService != null && mService.isPlaying()) {
                mPauseButton.setImageResource(R.drawable.ic_av_pause_over_video_large_light);
            } else {
                mPauseButton.setImageResource(R.drawable.ic_av_play_over_video_large_light);
            }
        } catch (RemoteException ex) {
        }
    }
    
    private void setSeekControls() {
    	if (mService == null) {
    		return;
    	}
    	
    	try {
			if (mService.duration() > 0) {
				mProgress.setEnabled(true);
			} else {
				mProgress.setEnabled(false);
			}
		} catch (RemoteException e) {
		}	
    }
    
    private void initPager() {
    	if (mService == null) {
    		return;
        }
     	
    	int count = 0;
    	
    	try {
			count = mService.getQueue().length + 2;
		} catch (RemoteException e) {
			finish();
		}
    	
    	mAdapter = new GridPagerAdapter(getSupportFragmentManager(), count);
        mPager.setAdapter(mAdapter);
        mPager.setOnPageChangeListener(new ViewPager.OnPageChangeListener() {

        	boolean mFromUser = false;
        	
			@Override
			public void onPageScrollStateChanged(int state) {
				if (state == ViewPager.SCROLL_STATE_DRAGGING) {
					mFromUser = true;
			    } else if (state == ViewPager.SCROLL_STATE_IDLE) {
			    	mFromUser = false;
			    }
			}

			@Override
			public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {
				
			}

			@Override
			public void onPageSelected(int position) {
				try {
					if (mFromUser) {
						position--;
						
						if (position == -1) {
							position = mService.getQueue().length - 1;
						} else if (position == mService.getQueue().length) {
							position = 0;
						}
						
						mService.setQueuePosition(position);
						mFromUser = false;
					}
				} catch (RemoteException e) {
					e.printStackTrace();
				}
			}
        });
        
        setPager();
    }
    
    private void setPager() {
        if (mService == null) {
            return;
        }
    	
        try {
        	int position = mService.getQueuePosition() + 1;
			mPager.setCurrentItem(position, false);
		} catch (RemoteException e) {
			e.printStackTrace();
		}
    }
    
    private TextView mCurrentTime;
    private TextView mTotalTime;
    private ProgressBar mProgress;
    private long mPosOverride = -1;
    private boolean mFromTouch = false;
    private long mDuration;
    private int seekmethod;
    private boolean paused;

    private static final int REFRESH = 1;
    private static final int QUIT = 2;

    private void queueNextRefresh(long delay) {
        if (!paused) {
            Message msg = mHandler.obtainMessage(REFRESH);
            mHandler.removeMessages(REFRESH);
            mHandler.sendMessageDelayed(msg, delay);
        }
    }

    private long refreshNow() {
        if(mService == null)
            return 500;
        try {
            long pos = mPosOverride < 0 ? mService.position() : mPosOverride;
            if ((pos >= 0)) {
                mCurrentTime.setText(MusicUtils.makeTimeString(this, pos / 1000));
                if (mDuration > 0) {
                	mProgress.setProgress((int) (1000 * pos / mDuration));
                } else {
                	mProgress.setProgress(1000);
                }
                
                if (mService.isPlaying()) {
                    mCurrentTime.setVisibility(View.VISIBLE);
                } else {
                    // blink the counter
                    int vis = mCurrentTime.getVisibility();
                    mCurrentTime.setVisibility(vis == View.INVISIBLE ? View.VISIBLE : View.INVISIBLE);
                    return 500;
                }
            } else {
                mCurrentTime.setText("--:--");
                mProgress.setProgress(1000);
            }
            // calculate the number of milliseconds until the next full second, so
            // the counter can be updated at just the right time
            long remaining = 1000 - (pos % 1000);

            // approximate how often we would need to refresh the slider to
            // move it smoothly
            int width = mProgress.getWidth();
            if (width == 0) width = 320;
            long smoothrefreshtime = mDuration / width;

            if (smoothrefreshtime > remaining) return remaining;
            if (smoothrefreshtime < 20) return 20;
            return smoothrefreshtime;
        } catch (RemoteException ex) {
        }
        return 500;
    }
    
    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case REFRESH:
                    long next = refreshNow();
                    queueNextRefresh(next);
                    break;
                    
                case QUIT:
                    // This can be moved back to onCreate once the bug that prevents
                    // Dialogs from being started from onCreate/onResume is fixed.
                    new AlertDialog.Builder(MediaPlayerActivity.this)
                            .setTitle(R.string.service_start_error_title)
                            .setMessage(R.string.service_start_error_msg)
                            .setPositiveButton(android.R.string.ok,
                                    new DialogInterface.OnClickListener() {
                                        public void onClick(DialogInterface dialog, int whichButton) {
                                            finish();
                                        }
                                    })
                            .setCancelable(false)
                            .show();
                    break;

                default:
                    break;
            }
        }
    };

    private BroadcastReceiver mStatusListener = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(MediaPlaybackService.META_CHANGED)) {
                // redraw the artist/title info and
                // set new max for progress bar
                updateTrackInfo();
                setSeekControls();
                setPauseButtonImage();
                queueNextRefresh(1);
                setPager();
            } else if (action.equals(MediaPlaybackService.PLAYSTATE_CHANGED)) {
                setPauseButtonImage();
            }
            /*else if (action.equals(MediaPlaybackService.START_DIALOG)) {
	        	if (mParentActivityState == VISIBLE) {
	        		showLoadingDialog();
	        	}
            } else if (action.equals(MediaPlaybackService.STOP_DIALOG)) {
            	if (mParentActivityState == VISIBLE) {
            		dismissLoadingDialog();
            	}
            }*/
        }
    };
    
    private void updateTrackInfo() {
        if (mService == null) {
            return;
        }
        try {
        	mDuration = mService.duration();
        	mTotalTime.setText(MusicUtils.makeTimeString(this, mDuration / 1000));
        } catch (RemoteException ex) {
        	finish();
        }
    }
    
	public synchronized void showLoadingDialog() {
		//mLoadingDialog = LoadingDialog.newInstance(this, getString(R.string.opening_url_message));
		//mLoadingDialog.show(getSupportFragmentManager(), LOADING_DIALOG);
	}
	
	public synchronized void dismissLoadingDialog() {
		if (mLoadingDialog != null) {
			mLoadingDialog.dismiss();
			mLoadingDialog = null;
		}
	}
	
	private class GridPagerAdapter extends FragmentStatePagerAdapter {
    	private int mCount;
		
		public GridPagerAdapter(FragmentManager fm, int count) {
    		super(fm);
    		mCount = count;
        }
    	
        @Override
        public Fragment getItem(int position) {
            return new MediaPlayerFragment();
        }

        @Override
        public int getCount() {
        	return mCount;
        }
        
        @Override
        public Parcelable saveState() {
        	return null;
        }
    }
}
