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

//#define LOG_NDEBUG 0
#define LOG_TAG "FFmpegMediaPlayer"
#include <android/log.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <Errors.h>
#include <mediaplayer.h>

extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "ffmpeg_mediaplayer.h"
}

using namespace std;

MediaPlayer::MediaPlayer()
{
    __android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "constructor");
	
    state = NULL;
    //::init(&state);
    
    mListener = NULL;
    mCookie = NULL;
    mDuration = -1;
    mStreamType = 3;
    mCurrentPosition = -1;
    mSeekPosition = -1;
    mCurrentState = MEDIA_PLAYER_IDLE;
    mPrepareSync = false;
    mPrepareStatus = NO_ERROR;
    mLoop = false;
    mLeftVolume = mRightVolume = 1.0;
    mVideoWidth = mVideoHeight = 0;
    //mLockThreadId = 0;
    mAudioSessionId = 0;
    mSendLevel = 0;
}

MediaPlayer::~MediaPlayer()
{
	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "destructor");
    disconnect();
    //IPCThreadState::self()->flushCommands();
}

void MediaPlayer::disconnect()
{
	
	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "disconnect");
    /*sp<IMediaPlayer> p;
    {
        Mutex::Autolock _l(mLock);
        p = mPlayer;
        mPlayer.clear();
    }*/

    if (state != 0) {
        ::disconnect(&state);
    }
}

// always call with lock held
void MediaPlayer::clear_l()
{
    mDuration = -1;
    mCurrentPosition = -1;
    mSeekPosition = -1;
    mVideoWidth = mVideoHeight = 0;
}

static void
notifyListener(void* clazz, int msg, int ext1, int ext2, int fromThread)
{
    MediaPlayer* mp = (MediaPlayer*) clazz;
    mp->notify(msg, ext1, ext2, fromThread);
}

static int
initAudioTrackListener(void* clazz, int sampleRateInHz, int channelConfig, int fromThread)
{
    MediaPlayer* mp = (MediaPlayer*) clazz;
    mp->initAudioTrack(sampleRateInHz, channelConfig, fromThread);
}

static void
writeAudioListener(void* clazz, int16_t *samples, int frame_size_ptr, int fromThread)
{
    MediaPlayer* mp = (MediaPlayer*) clazz;
    mp->writeAudio(samples, frame_size_ptr, fromThread);
}

status_t MediaPlayer::setListener(MediaPlayerListener *listener)
{
    __android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "setListener");
    //Mutex::Autolock _l(mLock);
    mListener = listener;
    if (state != 0) {
    	::setNotifyListener(&state, this, notifyListener);
    	::setInitAudioTrackListener(&state, this, initAudioTrackListener);
    	::setWriteAudioListener(&state, this, writeAudioListener);
    }
    return NO_ERROR;
}

MediaPlayerListener * MediaPlayer::getListener()
{
    return mListener;
}

status_t MediaPlayer::setDataSource(State *player)
{
    status_t err = UNKNOWN_ERROR;
    State *p;
    { // scope for the lock
        //Mutex::Autolock _l(mLock);

        if ( !( (mCurrentState & MEDIA_PLAYER_IDLE) ||
                (mCurrentState == MEDIA_PLAYER_STATE_ERROR ) ) ) {
        	__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "setDataSource called in state %d", mCurrentState);
            return INVALID_OPERATION;
        }

        clear_l();
        p = state;
        state = player;
        if (player != 0) {
            mCurrentState = MEDIA_PLAYER_INITIALIZED;
            err = NO_ERROR;
        } else {
        	__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Unable to to create media player");
        }
    }

    if (p != 0) {
    	::disconnect(&p);
    }

    return err;
}

status_t MediaPlayer::setDataSource(const char *url, const char *headers)
{
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "setDataSource(%s)", url);
    status_t err = BAD_VALUE;
    if (url != NULL) {
        //const sp<IMediaPlayerService>& service(getMediaPlayerService());
        //if (service != 0) {
            //sp<IMediaPlayer> player(
            //        service->create(getpid(), this, url, headers, mAudioSessionId));
    	    State *state = NULL;
    	    ::init(&state);
    	    ::setNotifyListener(&state, this, notifyListener);
    	    ::setInitAudioTrackListener(&state, this, initAudioTrackListener);
    	    ::setWriteAudioListener(&state, this, writeAudioListener);
    	    err = ::setDataSourceURI(&state, url, headers);
    	    if (err == NO_ERROR) {
    	    	err = setDataSource(state);
    	    }
        //}
    }
    return err;
}

status_t MediaPlayer::setDataSource(int fd, int64_t offset, int64_t length)
{
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "setDataSource(%d, %lld, %lld)", fd, offset, length);
    status_t err = UNKNOWN_ERROR;
    //const sp<IMediaPlayerService>& service(getMediaPlayerService());
    //if (state != 0) {
        //sp<IMediaPlayer> player(service->create(getpid(), this, fd, offset, length, mAudioSessionId));
    	State *state = NULL;
    	::init(&state);
    	::setNotifyListener(&state, this, notifyListener);
    	::setInitAudioTrackListener(&state, this, initAudioTrackListener);
    	::setWriteAudioListener(&state, this, writeAudioListener);
    	err = ::setDataSourceFD(&state, fd, offset, length);
        err = setDataSource(state);
    //}
    return err;
}

status_t MediaPlayer::setMetadataFilter(char *allow[], char *block[])
{
	__android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, "setMetadataFilter");
    //Mutex::Autolock lock(mLock);
    if (state == NULL) {
        return NO_INIT;
    }
    return ::setMetadataFilter(&state, allow, block);
}

status_t MediaPlayer::getMetadata(const char *key, char **value)
{
    __android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, "getMetadata");
    //Mutex::Autolock lock(mLock);
    if (state == NULL) {
        return NO_INIT;
    }
    return ::get_metadata(&state, key, value);
}

/*status_t MediaPlayer::setVideoSurface(const sp<Surface>& surface)
{
    LOGV("setVideoSurface");
     Mutex::Autolock _l(mLock);
     if (mPlayer == 0) return NO_INIT;
     if (surface != NULL)
     return  mPlayer->setVideoSurface(surface->getISurface());
     else
    return  mPlayer->setVideoSurface(NULL);
}*/

// must call with lock held
int MediaPlayer::prepareAsync_l()
{
    if ( (state != 0) && ( mCurrentState & ( MEDIA_PLAYER_INITIALIZED | MEDIA_PLAYER_STOPPED) ) ) {
        setAudioStreamType(mStreamType);
        mCurrentState = MEDIA_PLAYER_PREPARING;
        return ::prepareAsync(&state);
    }
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "prepareAsync called in state %d", mCurrentState);
    return INVALID_OPERATION;
}

// TODO: In case of error, prepareAsync provides the caller with 2 error codes,
// one defined in the Android framework and one provided by the implementation
// that generated the error. The sync version of prepare returns only 1 error
// code.
status_t MediaPlayer::prepare()
{
    __android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "prepare");
     /*Mutex::Autolock _l(mLock);
     mLockThreadId = getThreadId();
     if (mPrepareSync) {
     mLockThreadId = 0;
     return -EALREADY;
     }
     mPrepareSync = true;
     int ret = prepareAsync_l();
     if (ret != NO_ERROR) {
     mLockThreadId = 0;
     return ret;
     }
     
     if (mPrepareSync) {
     mSignal.wait(mLock);  // wait for prepare done
     mPrepareSync = false;
     }
     LOGV("prepare complete - status=%d", mPrepareStatus);
     mLockThreadId = 0;
     return mPrepareStatus;*/
    return ::prepare(&state);
}

status_t MediaPlayer::prepareAsync()
{
    __android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "prepareAsync");
    //Mutex::Autolock _l(mLock);
    return ::prepareAsync(&state);
}

status_t MediaPlayer::start()
{
    __android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "start");
    //Mutex::Autolock _l(mLock);
    if (mCurrentState & MEDIA_PLAYER_STARTED)
        return NO_ERROR;
    if ( (state != 0) && ( mCurrentState & ( MEDIA_PLAYER_PREPARED |
                    MEDIA_PLAYER_PLAYBACK_COMPLETE | MEDIA_PLAYER_PAUSED ) ) ) {
        setLooping(mLoop);
        setVolume(mLeftVolume, mRightVolume);
        setAuxEffectSendLevel(mSendLevel);
        mCurrentState = MEDIA_PLAYER_STARTED;
        status_t ret = ::start(&state);
        if (ret != NO_ERROR) {
            mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        } else {
            if (mCurrentState == MEDIA_PLAYER_PLAYBACK_COMPLETE) {
            	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "playback completed immediately following start()");
            }
        }
        return ret;
    }
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "start called in state %d", mCurrentState);
    return INVALID_OPERATION;
}

status_t MediaPlayer::stop()
{
	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "stop");
    //Mutex::Autolock _l(mLock);
    if (mCurrentState & MEDIA_PLAYER_STOPPED) return NO_ERROR;
    if ( (state != 0) && ( mCurrentState & ( MEDIA_PLAYER_STARTED | MEDIA_PLAYER_PREPARED |
                    MEDIA_PLAYER_PAUSED | MEDIA_PLAYER_PLAYBACK_COMPLETE ) ) ) {
        status_t ret = ::stop(&state);
        if (ret != NO_ERROR) {
            mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        } else {
            mCurrentState = MEDIA_PLAYER_STOPPED;
        }
        return ret;
    }
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "stop called in state %d", mCurrentState);
    return INVALID_OPERATION;
}

status_t MediaPlayer::pause()
{
	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "pause");
    //Mutex::Autolock _l(mLock);
    if (mCurrentState & (MEDIA_PLAYER_PAUSED|MEDIA_PLAYER_PLAYBACK_COMPLETE))
        return NO_ERROR;
    if ((state != 0) && (mCurrentState & MEDIA_PLAYER_STARTED)) {
        status_t ret = ::pause(&state);
        if (ret != NO_ERROR) {
            mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        } else {
            mCurrentState = MEDIA_PLAYER_PAUSED;
        }
        return ret;
    }
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "pause called in state %d", mCurrentState);
    return INVALID_OPERATION;
}

bool MediaPlayer::isPlaying()
{
    //Mutex::Autolock _l(mLock);
    if (state != 0) {
        bool temp = false;
        //mPlayer->isPlaying(&temp); // TODO fix this!
        if (::isPlaying(&state)) {
        	temp = true;
        }
        __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "isPlaying: %d", temp);
        if ((mCurrentState & MEDIA_PLAYER_STARTED) && ! temp) {
        	__android_log_write(ANDROID_LOG_ERROR, LOG_TAG, "internal/external state mismatch corrected");
            mCurrentState = MEDIA_PLAYER_PAUSED;
        }
        return temp;
    }
    __android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "isPlaying: no active player");
    return false;
}

status_t MediaPlayer::getVideoWidth(int *w)
{
	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "getVideoWidth");
    //Mutex::Autolock _l(mLock);
    if (state == 0) return INVALID_OPERATION;
    *w = mVideoWidth;
    return NO_ERROR;
}

status_t MediaPlayer::getVideoHeight(int *h)
{
	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "getVideoHeight");
    //Mutex::Autolock _l(mLock);
    if (state == 0) return INVALID_OPERATION;
    *h = mVideoHeight;
    return NO_ERROR;
}

status_t MediaPlayer::getCurrentPosition(int *msec)
{
	//__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "getCurrentPosition");
    //Mutex::Autolock _l(mLock);
    if (state != 0) {
        if (mCurrentPosition >= 0) {
        	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "Using cached seek position: %d", mCurrentPosition);
            *msec = mCurrentPosition;
            return NO_ERROR;
        }
        return ::getCurrentPosition(&state, msec);
    }
    return INVALID_OPERATION;
}

status_t MediaPlayer::getDuration_l(int *msec)
{
	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "getDuration");
    bool isValidState = (mCurrentState & (MEDIA_PLAYER_PREPARED | MEDIA_PLAYER_STARTED | MEDIA_PLAYER_PAUSED | MEDIA_PLAYER_STOPPED | MEDIA_PLAYER_PLAYBACK_COMPLETE));
    if (state != 0 && isValidState) {
        status_t ret = NO_ERROR;
        if (mDuration <= 0)
            ret = ::getDuration(&state, &mDuration);
        if (msec)
            *msec = mDuration;
        return ret;
    }
    __android_log_write(ANDROID_LOG_ERROR, LOG_TAG, "Attempt to call getDuration without a valid mediaplayer");
    return INVALID_OPERATION;
}

status_t MediaPlayer::getDuration(int *msec)
{
    //Mutex::Autolock _l(mLock);
    return getDuration_l(msec);
}

status_t MediaPlayer::seekTo_l(int msec)
{
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "seekTo %d", msec);
    if ((state != 0) && ( mCurrentState & ( MEDIA_PLAYER_STARTED | MEDIA_PLAYER_PREPARED | MEDIA_PLAYER_PAUSED | MEDIA_PLAYER_PLAYBACK_COMPLETE) ) ) {
        if ( msec < 0 ) {
        	__android_log_print(ANDROID_LOG_WARN, LOG_TAG, "Attempt to seek to invalid position: %d", msec);
            msec = 0;
        } else if ((mDuration > 0) && (msec > mDuration)) {
        	__android_log_print(ANDROID_LOG_WARN, LOG_TAG, "Attempt to seek to past end of file: request = %d, EOF = %d", msec, mDuration);
            msec = mDuration;
        }
        // cache duration
        mCurrentPosition = msec;
        if (mSeekPosition < 0) {
            getDuration_l(NULL);
            mSeekPosition = msec;
            return ::seekTo(&state, msec);
        }
        else {
        	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "Seek in progress - queue up seekTo[%d]", msec);
            return NO_ERROR;
        }
    }
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Attempt to perform seekTo in wrong state: mPlayer=%p, mCurrentState=%u", state, mCurrentState);
    return INVALID_OPERATION;
}

status_t MediaPlayer::seekTo(int msec)
{
    //mLockThreadId = getThreadId();
    //Mutex::Autolock _l(mLock);
    status_t result = seekTo_l(msec);
    //mLockThreadId = 0;

    return result;
}

status_t MediaPlayer::reset()
{
	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "reset");
    //Mutex::Autolock _l(mLock);
    mLoop = false;
    if (mCurrentState == MEDIA_PLAYER_IDLE) return NO_ERROR;
    mPrepareSync = false;
    if (state != 0) {
        status_t ret = ::reset(&state);
        if (ret != NO_ERROR) {
        	__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "reset() failed with return code (%d)", ret);
            mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        } else {
            mCurrentState = MEDIA_PLAYER_IDLE;
        }
        return ret;
    }
    clear_l();
    return NO_ERROR;
}

status_t MediaPlayer::setAudioStreamType(int type)
{
	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "MediaPlayer::setAudioStreamType");
    //Mutex::Autolock _l(mLock);
    if (mStreamType == type) return NO_ERROR;
    if (mCurrentState & ( MEDIA_PLAYER_PREPARED | MEDIA_PLAYER_STARTED |
                MEDIA_PLAYER_PAUSED | MEDIA_PLAYER_PLAYBACK_COMPLETE ) ) {
        // Can't change the stream type after prepare
    	__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "setAudioStream called in state %d", mCurrentState);
        return INVALID_OPERATION;
    }
    // cache
    mStreamType = type;
    return OK;
}

status_t MediaPlayer::setLooping(int loop)
{
	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "MediaPlayer::setLooping");
    //Mutex::Autolock _l(mLock);
    mLoop = (loop != 0);
    if (state != 0) {
        return ::setLooping(&state, loop);
    }
    return OK;
}

bool MediaPlayer::isLooping() {
	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "isLooping");
    //Mutex::Autolock _l(mLock);
    if (state != 0) {
        return mLoop;
    }
    __android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "isLooping: no active player");
    return false;
}

status_t MediaPlayer::setVolume(float leftVolume, float rightVolume)
{
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "MediaPlayer::setVolume(%f, %f)", leftVolume, rightVolume);
    //Mutex::Autolock _l(mLock);
    mLeftVolume = leftVolume;
    mRightVolume = rightVolume;
    if (state != 0) {
    	 MediaPlayerListener *listener = mListener;
    	 if (listener != 0) {
    		 return listener->setVolume(leftVolume, rightVolume);
    	 }
    }
    return OK;
}

status_t MediaPlayer::setAudioSessionId(int sessionId)
{
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "MediaPlayer::setAudioSessionId(%d)", sessionId);
    //Mutex::Autolock _l(mLock);
    if (!(mCurrentState & MEDIA_PLAYER_IDLE)) {
    	__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "setAudioSessionId called in state %d", mCurrentState);
        return INVALID_OPERATION;
    }
    if (sessionId < 0) {
        return BAD_VALUE;
    }
    mAudioSessionId = sessionId;
    return NO_ERROR;
}

int MediaPlayer::getAudioSessionId()
{
    //Mutex::Autolock _l(mLock);
    return mAudioSessionId;
}

status_t MediaPlayer::setAuxEffectSendLevel(float level)
{
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "MediaPlayer::setAuxEffectSendLevel(%f)", level);
    //Mutex::Autolock _l(mLock);
    mSendLevel = level;
    if (state != 0) {
        MediaPlayerListener *listener = mListener;
        if (listener != 0) {
        	return listener->setAuxEffectSendLevel(level);
        }
    }
    return OK;
}

status_t MediaPlayer::attachAuxEffect(int effectId)
{
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "MediaPlayer::attachAuxEffect(%d)", effectId);
    //Mutex::Autolock _l(mLock);
    if (state == 0 ||
        (mCurrentState & MEDIA_PLAYER_IDLE) ||
        (mCurrentState == MEDIA_PLAYER_STATE_ERROR )) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "attachAuxEffect called in state %d", mCurrentState);
        return INVALID_OPERATION;
    }

    MediaPlayerListener *listener = mListener;
    if (listener != 0) {
    	return listener->attachAuxEffect(effectId);
    } else {
    	return -3;
    }
}

void MediaPlayer::notify(int msg, int ext1, int ext2, int fromThread)
{
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "message received msg=%d, ext1=%d, ext2=%d", msg, ext1, ext2);
    bool send = true;
    bool locked = false;

    // TODO: In the future, we might be on the same thread if the app is
    // running in the same process as the media server. In that case,
    // this will deadlock.
    //
    // The threadId hack below works around this for the care of prepare
    // and seekTo within the same process.
    // FIXME: Remember, this is a hack, it's not even a hack that is applied
    // consistently for all use-cases, this needs to be revisited.
    /*if (mLockThreadId != getThreadId()) {
        mLock.lock();
        locked = true;
    }*/

    // Allows calls from JNI in idle state to notify errors
    if (!(msg == MEDIA_ERROR && mCurrentState == MEDIA_PLAYER_IDLE) && state == 0) {
    	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "notify(%d, %d, %d) callback on disconnected mediaplayer", msg, ext1, ext2);
        //if (locked) mLock.unlock(); // release the lock when done.
        return;
    }

    switch (msg) {
    case MEDIA_NOP: // interface test message
        break;
    case MEDIA_PREPARED:
    	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "prepared");
        mCurrentState = MEDIA_PLAYER_PREPARED;
        if (mPrepareSync) {
        	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "signal application thread");
            mPrepareSync = false;
            mPrepareStatus = NO_ERROR;
            //mSignal.signal();
        }
        break;
    case MEDIA_PLAYBACK_COMPLETE:
    	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "playback complete");
        if (mCurrentState == MEDIA_PLAYER_IDLE) {
        	__android_log_write(ANDROID_LOG_ERROR, LOG_TAG, "playback complete in idle state");
        }
        if (!mLoop) {
            mCurrentState = MEDIA_PLAYER_PLAYBACK_COMPLETE;
        }
        break;
    case MEDIA_ERROR:
        // Always log errors.
        // ext1: Media framework error code.
        // ext2: Implementation dependant error code.
    	__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "error (%d, %d)", ext1, ext2);
        mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        if (mPrepareSync)
        {
        	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "signal application thread");
            mPrepareSync = false;
            mPrepareStatus = ext1;
            //mSignal.signal();
            send = false;
        }
        break;
    /*case MEDIA_INFO:
        // ext1: Media framework error code.
        // ext2: Implementation dependant error code.
    	__android_log_print(ANDROID_LOG_WARN, LOG_TAG, "info/warning (%d, %d)", ext1, ext2);
        break;*/
    case MEDIA_SEEK_COMPLETE:
    	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "Received seek complete");
        if (mSeekPosition != mCurrentPosition) {
        	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "Executing queued seekTo(%d)", mSeekPosition);
            mSeekPosition = -1;
            seekTo_l(mCurrentPosition);
        }
        else {
        	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "All seeks complete - return to regularly scheduled program");
            mCurrentPosition = mSeekPosition = -1;
        }
        break;
    case MEDIA_BUFFERING_UPDATE:
    	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "buffering %d", ext1);
        break;
    /*case MEDIA_SET_VIDEO_SIZE:
    	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "New video size %d x %d", ext1, ext2);
        mVideoWidth = ext1;
        mVideoHeight = ext2;
        break;*/
    default:
    	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "unrecognized message: (%d, %d, %d)", msg, ext1, ext2);
        break;
    }

    MediaPlayerListener *listener = mListener;
    //if (locked) mLock.unlock();

    // this prevents re-entrant calls into client code
    if ((listener != 0) && send) {
        //Mutex::Autolock _l(mNotifyLock);
        __android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "callback application");
        listener->notify(msg, ext1, ext2, fromThread);
        __android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "back from callback");
    }
}

int MediaPlayer::setNextMediaPlayer(const MediaPlayer* next) {
    if (state == NULL) {
        return NO_INIT;
    }
    //return mPlayer->setNextPlayer(next == NULL ? NULL : next->mPlayer);
    return 0;
}

int MediaPlayer::initAudioTrack(int sampleRateInHz, int channelConfig, int fromThread)
{
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "initAudioTrack(%d, %d, %d)", sampleRateInHz, channelConfig, fromThread);
    
    MediaPlayerListener* listener = mListener;

    int bufferSize = 0;
     
    if ((listener != 0)) {
    	bufferSize = listener->initAudioTrack(mStreamType, sampleRateInHz, channelConfig, mAudioSessionId, fromThread);
    }
     
    return bufferSize;
}

void MediaPlayer::writeAudio(int16_t *samples, int frame_size_ptr, int fromThread)
{
	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "MediaPlayer::writeAudio");
    
    MediaPlayerListener* listener = mListener;

    if ((listener != 0)) {
    	listener->writeAudio(samples, frame_size_ptr, fromThread);
    }
}

/*static*/ /*sp<IMemory> MediaPlayer::decode(const char* url, uint32_t *pSampleRate, int* pNumChannels, int* pFormat)
{
    LOGV("decode(%s)", url);
     sp<IMemory> p;
     const sp<IMediaPlayerService>& service = getMediaPlayerService();
     if (service != 0) {
     p = service->decode(url, pSampleRate, pNumChannels, pFormat);
     } else {
     LOGE("Unable to locate media service");
     }
    return p;
    
}*/
