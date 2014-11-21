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

/* This is a JNI example where we use native methods to play sounds
 * using OpenSL ES. See the corresponding Java source file located at:
 *
 *   src/com/example/nativeaudio/NativeAudio/NativeAudio.java
 */

#include <assert.h>
#include <string.h>

const int JNI_TRUE = 1;
const JNI_FALSE = 0;

// for __android_log_print(ANDROID_LOG_INFO, "YourApp", "formatted message");
#include <android/log.h>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

// for native asset manager
#include <sys/types.h>

#include <player.h>

#define SAWTOOTH_FRAMES 8000
static short sawtoothBuffer[SAWTOOTH_FRAMES];


// synthesize a mono sawtooth wave and place it into a buffer (called automatically on load)
__attribute__((constructor)) static void onDlOpen(void)
{
    unsigned i;
    for (i = 0; i < SAWTOOTH_FRAMES; ++i) {
        sawtoothBuffer[i] = 32768 - ((i % 100) * 660);
    }
}


// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
    Player *player = (Player *) context;

    assert(bq == player->bqPlayerBufferQueue);

player->nextBuffer = sawtoothBuffer;
player->nextSize = sizeof(sawtoothBuffer);
player->nextCount = 2;

    //assert(NULL == context);
    // for streaming playback, replace this test by logic to find and fill the next buffer
    if (--player->nextCount > 0 && NULL != player->nextBuffer && 0 != player->nextSize) {
player->nextCount = 1;
        SLresult result;
        // enqueue another buffer
        result = (*player->bqPlayerBufferQueue)->Enqueue(player->bqPlayerBufferQueue, player->nextBuffer, player->nextSize);
        // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
        // which for this code example would indicate a programming error
        assert(SL_RESULT_SUCCESS == result);
        (void)result;
    }
}

// create the engine and output mix objects
void createEngine(Player **ps)
{
    Player *player = *ps;
    
    SLresult result;

    // create engine
    result = slCreateEngine(&player->engineObject, 0, NULL, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize the engine
    result = (*player->engineObject)->Realize(player->engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the engine interface, which is needed in order to create other objects
    result = (*player->engineObject)->GetInterface(player->engineObject, SL_IID_ENGINE, &player->engineEngine);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // create output mix, with environmental reverb specified as a non-required interface
    const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    result = (*player->engineEngine)->CreateOutputMix(player->engineEngine, &player->outputMixObject, 1, ids, req);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize the output mix
    result = (*player->outputMixObject)->Realize(player->outputMixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the environmental reverb interface
    // this could fail if the environmental reverb effect is not available,
    // either because the feature is not present, excessive CPU load, or
    // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
    result = (*player->outputMixObject)->GetInterface(player->outputMixObject, SL_IID_ENVIRONMENTALREVERB,
            &player->outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS == result) {
        result = (*player->outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                player->outputMixEnvironmentalReverb, &reverbSettings);
        (void)result;
    }
    // ignore unsuccessful result codes for environmental reverb, as it is optional for this example

}


// create buffer queue audio player
void createBufferQueueAudioPlayer(Player **ps)
{
    Player *player = *ps;
    
    SLresult result;

    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_8,
        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, player->outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    // create audio player
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
            /*SL_IID_MUTESOLO,*/ SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
            /*SL_BOOLEAN_TRUE,*/ SL_BOOLEAN_TRUE};
    result = (*player->engineEngine)->CreateAudioPlayer(player->engineEngine, &player->bqPlayerObject, &audioSrc, &audioSnk,
            3, ids, req);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize the player
    result = (*player->bqPlayerObject)->Realize(player->bqPlayerObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the play interface
    result = (*player->bqPlayerObject)->GetInterface(player->bqPlayerObject, SL_IID_PLAY, &player->bqPlayerPlay);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the buffer queue interface
    result = (*player->bqPlayerObject)->GetInterface(player->bqPlayerObject, SL_IID_BUFFERQUEUE,
            &player->bqPlayerBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // register callback on the buffer queue
    result = (*player->bqPlayerBufferQueue)->RegisterCallback(player->bqPlayerBufferQueue, bqPlayerCallback, player);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the effect send interface
    result = (*player->bqPlayerObject)->GetInterface(player->bqPlayerObject, SL_IID_EFFECTSEND,
            &player->bqPlayerEffectSend);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

#if 0   // mute/solo is not supported for sources that are known to be mono, as this is
    // get the mute/solo interface
    result = (*player->bqPlayerObject)->GetInterface(player->bqPlayerObject, SL_IID_MUTESOLO, &player->bqPlayerMuteSolo);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
#endif

    // get the volume interface
    result = (*player->bqPlayerObject)->GetInterface(player->bqPlayerObject, SL_IID_VOLUME, &player->bqPlayerVolume);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // set the player's state to playing
    //result = (*player->bqPlayerPlay)->SetPlayState(player->bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    //assert(SL_RESULT_SUCCESS == result);
    //(void)result;
}


// set the playing state for the buffer queue audio player
// to PLAYING (true) or PAUSED (false)
void setPlayingAudioPlayer(Player **ps, int isPlaying)
{
    Player *player = *ps;

    SLresult result;

    // make sure the URI audio player was created
    if (NULL != player->bqPlayerPlay) {

        // set the player's state
        result = (*player->bqPlayerPlay)->SetPlayState(player->bqPlayerPlay, isPlaying ?
            SL_PLAYSTATE_PLAYING : SL_PLAYSTATE_PAUSED);
        assert(SL_RESULT_SUCCESS == result);
        (void)result;
    }

}


// expose the mute/solo APIs to Java for one of the 3 players

SLMuteSoloItf getMuteSolo(Player *player)
{
    return player->bqPlayerMuteSolo;
}

void setChannelMuteUriAudioPlayer(Player **ps, int chan, int mute)
{
    Player *player = *ps;
    
    SLresult result;
    SLMuteSoloItf muteSoloItf = getMuteSolo(player);
    if (NULL != muteSoloItf) {
        result = (*muteSoloItf)->SetChannelMute(muteSoloItf, chan, mute);
        assert(SL_RESULT_SUCCESS == result);
        (void)result;
    }
}

void setChannelSoloUriAudioPlayer(Player **ps, int chan, int solo)
{
    Player *player = *ps;
    
    SLresult result;
    SLMuteSoloItf muteSoloItf = getMuteSolo(player);
    if (NULL != muteSoloItf) {
        result = (*muteSoloItf)->SetChannelSolo(muteSoloItf, chan, solo);
        assert(SL_RESULT_SUCCESS == result);
        (void)result;
    }
}

int getNumChannelsUriAudioPlayer(Player **ps)
{
    Player *player = *ps;
    
    SLuint8 numChannels;
    SLresult result;
    SLMuteSoloItf muteSoloItf = getMuteSolo(player);
    if (NULL != muteSoloItf) {
        result = (*muteSoloItf)->GetNumChannels(muteSoloItf, &numChannels);
        if (SL_RESULT_PRECONDITIONS_VIOLATED == result) {
            // channel count is not yet known
            numChannels = 0;
        } else {
            assert(SL_RESULT_SUCCESS == result);
        }
    } else {
        numChannels = 0;
    }
    return numChannels;
}

// expose the volume APIs to Java for one of the 3 players

SLVolumeItf getVolume(Player *player)
{
    return player->bqPlayerVolume;
}

void setVolumeUriAudioPlayer(Player **ps, int millibel)
{
    Player *player = *ps;
    
    SLresult result;
    SLVolumeItf volumeItf = getVolume(player);
    if (NULL != volumeItf) {
        result = (*volumeItf)->SetVolumeLevel(volumeItf, millibel);
        assert(SL_RESULT_SUCCESS == result);
        (void)result;
    }
}

void setMuteUriAudioPlayer(Player **ps, int mute)
{
    Player *player = *ps;
    
    SLresult result;
    SLVolumeItf volumeItf = getVolume(player);
    if (NULL != volumeItf) {
        result = (*volumeItf)->SetMute(volumeItf, mute);
        assert(SL_RESULT_SUCCESS == result);
        (void)result;
    }
}

void enableStereoPositionUriAudioPlayer(Player **ps, int enable)
{
    Player *player = *ps;
    
    SLresult result;
    SLVolumeItf volumeItf = getVolume(player);
    if (NULL != volumeItf) {
        result = (*volumeItf)->EnableStereoPosition(volumeItf, enable);
        assert(SL_RESULT_SUCCESS == result);
        (void)result;
    }
}

void setStereoPositionUriAudioPlayer(Player **ps, int permille)
{
    Player *player = *ps;
    
    SLresult result;
    SLVolumeItf volumeItf = getVolume(player);
    if (NULL != volumeItf) {
        result = (*volumeItf)->SetStereoPosition(volumeItf, permille);
        assert(SL_RESULT_SUCCESS == result);
        (void)result;
    }
}

// enable reverb on the buffer queue player
int enableReverb(Player **ps, int enabled)
{
    Player *player = *ps;
    
    SLresult result;

    // we might not have been able to add environmental reverb to the output mix
    if (NULL == player->outputMixEnvironmentalReverb) {
        return JNI_FALSE;
    }

    result = (*player->bqPlayerEffectSend)->EnableEffectSend(player->bqPlayerEffectSend,
            player->outputMixEnvironmentalReverb, (SLboolean) enabled, (SLmillibel) 0);
    // and even if environmental reverb was present, it might no longer be available
    if (SL_RESULT_SUCCESS != result) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}


// select the desired clip and play count, and enqueue the first buffer if idle
int selectClip(Player **ps, int which, int count)
{
    Player *player = *ps;
    
    switch (which) {
    case 0:     // CLIP_NONE
        player->nextBuffer = (short *) NULL;
        player->nextSize = 0;
        break;
    case 1:     // CLIP_HELLO
        break;
    case 2:     // CLIP_ANDROID
        break;
    case 3:     // CLIP_SAWTOOTH
        player->nextBuffer = sawtoothBuffer;
        player->nextSize = sizeof(sawtoothBuffer);
        break;
    case 4:     // CLIP_PLAYBACK
        // we recorded at 16 kHz, but are playing buffers at 8 Khz, so do a primitive down-sample
        /*if (recorderSR == SL_SAMPLINGRATE_16) {
            unsigned i;
            for (i = 0; i < recorderSize; i += 2 * sizeof(short)) {
                recorderBuffer[i >> 2] = recorderBuffer[i >> 1];
            }
            recorderSR = SL_SAMPLINGRATE_8;
            recorderSize >>= 1;
        }
        player->nextBuffer = recorderBuffer;
        player->nextSize = recorderSize;*/
        break;
    default:
        player->nextBuffer = NULL;
        player->nextSize = 0;
        break;
    }
    player->nextCount = count;
    if (player->nextSize > 0) {
        // here we only enqueue one buffer because it is a long clip,
        // but for streaming playback we would typically enqueue at least 2 buffers to start
        SLresult result;
        result = (*player->bqPlayerBufferQueue)->Enqueue(player->bqPlayerBufferQueue, player->nextBuffer, player->nextSize);
        if (SL_RESULT_SUCCESS != result) {
            return JNI_FALSE;
        }
    }

    return JNI_TRUE;
}


// shut down the native audio system
void shutdown(Player **ps)
{
    Player *player = *ps;

    // destroy buffer queue audio player object, and invalidate all associated interfaces
    if (player->bqPlayerObject != NULL) {
        (*player->bqPlayerObject)->Destroy(player->bqPlayerObject);
        player->bqPlayerObject = NULL;
        player->bqPlayerPlay = NULL;
        player->bqPlayerBufferQueue = NULL;
        player->bqPlayerEffectSend = NULL;
        player->bqPlayerMuteSolo = NULL;
        player->bqPlayerVolume = NULL;
    }

    // destroy output mix object, and invalidate all associated interfaces
    if (player->outputMixObject != NULL) {
        (*player->outputMixObject)->Destroy(player->outputMixObject);
        player->outputMixObject = NULL;
        player->outputMixEnvironmentalReverb = NULL;
    }

    // destroy engine object, and invalidate all associated interfaces
    if (player->engineObject != NULL) {
        (*player->engineObject)->Destroy(player->engineObject);
        player->engineObject = NULL;
        player->engineEngine = NULL;
    }

}
