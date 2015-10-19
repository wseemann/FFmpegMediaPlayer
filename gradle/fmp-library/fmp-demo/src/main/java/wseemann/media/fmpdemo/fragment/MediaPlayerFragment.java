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

package wseemann.media.fmpdemo.fragment;

import wseemann.media.fmpdemo.R;
import wseemann.media.fmpdemo.service.IMediaPlaybackService;
import wseemann.media.fmpdemo.service.MediaPlaybackService;
import wseemann.media.fmpdemo.service.MusicUtils;
import wseemann.media.fmpdemo.service.MusicUtils.ServiceToken;
import wseemann.media.fmpdemo.view.CoverView;
import wseemann.media.fmpdemo.view.CoverView.CoverViewListener;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

public class MediaPlayerFragment extends Fragment implements CoverViewListener {
	
    private Worker mAlbumArtWorker;
    private AlbumArtHandler mAlbumArtHandler;
    private IMediaPlaybackService mService = null;
    
    private ServiceToken mToken;

    private TextView mTrackNumber;
    
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		View view = inflater.inflate(R.layout.fragment_media_player, container, false);
		
		mAlbumArtWorker = new Worker("album art worker");
        mAlbumArtHandler = new AlbumArtHandler(mAlbumArtWorker.getLooper());
        
        mAlbum = (CoverView) view.findViewById(R.id.album_art);
        mAlbum.setup(mAlbumArtWorker.getLooper(), this);
        mTrackName = (TextView) view.findViewById(R.id.trackname);
        mTrackName.setSelected(true);
        mArtistAndAlbumName = (TextView) view.findViewById(R.id.artist_and_album);
        mArtistAndAlbumName.setSelected(true);
        mTrackNumber = (TextView) view.findViewById(R.id.track_number_text);
		
		return view;
	}
    
    @Override
    public void onStart() {
        super.onStart();
        
        mToken = MusicUtils.bindToService(getActivity(), osc);
        if (mToken == null) {
            // something went wrong
            //mHandler.sendEmptyMessage(QUIT);
        }
        
        IntentFilter f = new IntentFilter();
        f.addAction(MediaPlaybackService.META_CHANGED);
        getActivity().registerReceiver(mStatusListener, new IntentFilter(f));
        updateTrackInfo();
    }
    
    @Override
    public void onResume() {
        super.onResume();
        updateTrackInfo();
    }

    @Override
    public void onStop() {
        getActivity().unregisterReceiver(mStatusListener);
        MusicUtils.unbindFromService(mToken);
        mService = null;
        super.onStop();
    }
    
    @Override
    public void onDestroy() {
        mAlbumArtWorker.quit();
        super.onDestroy();
    }
    
    private ServiceConnection osc = new ServiceConnection() {
        public void onServiceConnected(ComponentName classname, IBinder obj) {
            mService = IMediaPlaybackService.Stub.asInterface(obj);
            try {
                // Assume something is playing when the service says it is,
                // but also if the audio ID is valid but the service is paused.
                if (mService.getAudioId() >= 0 || mService.isPlaying() ||
                        mService.getPath() != null) {
                    // something is playing now, we're done
                    return;
                }
            } catch (RemoteException ex) {
            }
            // Service is dead or not playing anything. Return to the previous
            // activity.
            getActivity().finish();
        }
        public void onServiceDisconnected(ComponentName classname) {
            mService = null;
        }
    };
    
    private CoverView mAlbum;
    private TextView mArtistAndAlbumName;
    private TextView mTrackName;

    private static final int GET_ALBUM_ART = 3;
    private static final int REFRESH_ALBUM_ART = 4;

    private BroadcastReceiver mStatusListener = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(MediaPlaybackService.META_CHANGED)) {
                // redraw the artist/title info and
                // set new max for progress bar
                updateTrackInfo();
            }
        }
    };
    
    private static class IdWrapper {
        public long id;
		IdWrapper(long id) {
            this.id = id;
        }
    }
    
    private void updateTrackInfo() {
        if (mService == null) {
            return;
        }
        try {
            String path = mService.getPath();
            if (path == null) {
                getActivity().finish();
                return;
            }
            
            //mTrackNumber.setText(mService.getTrackNumber());
            
            String trackName = mService.getTrackName();
            String UNKNOWN_STRING = "Unknown";
            if (trackName == null || trackName.equals(UNKNOWN_STRING)) {
            	trackName = mService.getMediaUri();
            }
            
            mTrackName.setText(trackName);
            
            String artistName = mService.getArtistName();
            String albumName = mService.getAlbumName();
            String artistAndAlbumName = "";
            
            if (artistName != null && !artistName.equals(UNKNOWN_STRING)) {
            	artistAndAlbumName = artistName;
            }
            
            if (albumName != null && !albumName.equals(UNKNOWN_STRING)) {
            	if (artistAndAlbumName.equals("")) {
            		artistAndAlbumName = albumName;
            	} else {
            		artistAndAlbumName = artistAndAlbumName + " - " + albumName;
            	}
            }
            
            mArtistAndAlbumName.setText(artistAndAlbumName);
            mAlbumArtHandler.obtainMessage(GET_ALBUM_ART, new IdWrapper(10)).sendToTarget();
        } catch (RemoteException ex) {
        }
    }
    
    public class AlbumArtHandler extends Handler {
        private long mId = -1;
        
        public AlbumArtHandler(Looper looper) {
            super(looper);
        }
        
        @Override
        public void handleMessage(Message msg)
        {
            long id = ((IdWrapper) msg.obj).id;
            
            if (((msg.what == GET_ALBUM_ART && mId != id)
            		|| msg.what == REFRESH_ALBUM_ART)
            		&& id >= 0) {
            	if (mAlbum.generateBitmap(id)) {
            		mId = id;
            	}
            }
        }
    }
    
    private static class Worker implements Runnable {
        private final Object mLock = new Object();
        private Looper mLooper;
        
        /**
         * Creates a worker thread with the given name. The thread
         * then runs a {@link android.os.Looper}.
         * @param name A name for the new thread
         */
        Worker(String name) {
            Thread t = new Thread(null, this, name);
            t.setPriority(Thread.MIN_PRIORITY);
            t.start();
            synchronized (mLock) {
                while (mLooper == null) {
                    try {
                        mLock.wait();
                    } catch (InterruptedException ex) {
                    }
                }
            }
        }
        
        public Looper getLooper() {
            return mLooper;
        }
        
        public void run() {
            synchronized (mLock) {
                Looper.prepare();
                mLooper = Looper.myLooper();
                mLock.notifyAll();
            }
            Looper.loop();
        }
        
        public void quit() {
            mLooper.quit();
        }
    }

	@Override
	public void onCoverViewInitialized() {
        if (mService == null) {
            return;
        }
        
        mAlbumArtHandler.removeMessages(GET_ALBUM_ART);
		mAlbumArtHandler.obtainMessage(GET_ALBUM_ART, new IdWrapper(10)).sendToTarget();
	}
}
