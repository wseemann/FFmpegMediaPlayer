/*
 * FFmpegMediaPlayer: A unified interface for playing audio files and streams.
 *
 * Copyright 2014 William Seemann
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <Errors.h>

extern "C" {
	#include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "ffmpeg_mediaplayer.h"
}

using namespace std;

/*enum media_event_type {
    MEDIA_NOP               = 0, // interface test message
    MEDIA_PREPARED          = 1,
    MEDIA_PLAYBACK_COMPLETE = 2,
    MEDIA_BUFFERING_UPDATE  = 3,
    MEDIA_SEEK_COMPLETE     = 4,
    MEDIA_ERROR             = 100,
};

typedef int media_error_type;
const media_error_type MEDIA_ERROR_UNKNOWN = 1;
const media_error_type MEDIA_ERROR_SERVER_DIED = 100;

enum media_player_states {
    MEDIA_PLAYER_STATE_ERROR        = 0,
    MEDIA_PLAYER_IDLE               = 1 << 0,
    MEDIA_PLAYER_INITIALIZED        = 1 << 1,
    MEDIA_PLAYER_PREPARING          = 1 << 2,
    MEDIA_PLAYER_PREPARED           = 1 << 3,
    MEDIA_PLAYER_STARTED            = 1 << 4,
    MEDIA_PLAYER_PAUSED             = 1 << 5,
    MEDIA_PLAYER_STOPPED            = 1 << 6,
    MEDIA_PLAYER_PLAYBACK_COMPLETE  = 1 << 7
};*/

// ----------------------------------------------------------------------------
// ref-counted object for callbacks
class MediaPlayerListener
{
public:
    virtual void notify(int msg, int ext1, int ext2, int fromThread) = 0;
    virtual int initAudioTrack(int streamType, int sampleRateInHz, int channelConfig, int sessionId, int fromThread) = 0;
    virtual void writeAudio(int16_t *samples, int frame_size_ptr, int fromThread) = 0;
    virtual int setVolume(float leftVolume, float rightVolume) = 0;
    virtual int attachAuxEffect(int effectId) = 0;
    virtual int setAuxEffectSendLevel(float level) = 0;
};

class MediaPlayer
{
public:
    MediaPlayer();
    ~MediaPlayer();

            void            disconnect();
            status_t        setDataSource(const char *url, const char *headers);
            status_t        setDataSource(int fd, int64_t offset, int64_t length);
            status_t        setMetadataFilter(char *allow[], char *block[]);
            status_t        getMetadata(const char *key, char **value);
            //status_t        setVideoSurface(const sp<Surface>& surface);
            status_t        setListener(MediaPlayerListener *listener);
            MediaPlayerListener * getListener();
            status_t        prepare();
            status_t        prepareAsync();
            status_t        start();
            status_t        stop();
            status_t        pause();
            bool            isPlaying();
            status_t        getVideoWidth(int *w);
            status_t        getVideoHeight(int *h);
            status_t        seekTo(int msec);
            status_t        getCurrentPosition(int *msec);
            status_t        getDuration(int *msec);
            status_t        reset();
            status_t        setAudioStreamType(int type);
            status_t        setLooping(int loop);
            bool            isLooping();
            status_t        setVolume(float leftVolume, float rightVolume);
            void            notify(int msg, int ext1, int ext, int fromThread);
            status_t        setAudioSessionId(int sessionId);
            int             getAudioSessionId();
            status_t        setAuxEffectSendLevel(float level);
            int             attachAuxEffect(int effectId);
            status_t        setNextMediaPlayer(const MediaPlayer* player);
            int             initAudioTrack(int sampleRateInHz, int channelConfig, int fromThread);
            void            writeAudio(int16_t *samples, int frame_size_ptr, int fromThread);

        
private:
            void            clear_l();
            status_t        seekTo_l(int msec);
            status_t        prepareAsync_l();
            status_t        getDuration_l(int *msec);
            status_t        setDataSource(State *state);
        
    //sp<IMediaPlayer>            mPlayer;
    //Mutex                       mLock;
    //Mutex                       mNotifyLock;
    //Condition                   mSignal;
    MediaPlayerListener*        mListener;
    void*                       mCookie;
    media_player_states         mCurrentState;
    int                         mDuration;
    int                         mCurrentPosition;
    int                         mSeekPosition;
    bool                        mPrepareSync;
    status_t                    mPrepareStatus;
    int                         mStreamType;
    bool                        mLoop;
    float                       mLeftVolume;
    float                       mRightVolume;
    int                         mVideoWidth;
    int                         mVideoHeight;
    int                         mAudioSessionId;
    float                       mSendLevel;
    State*                      state;
    };

#endif // MEDIAPLAYER_H
