/*
 * FFmpegMediaPlayer: A unified interface for playing audio files and streams.
 *
 * Copyright 2016 William Seemann
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

#ifndef AUDIOPLAYER_H_
#define AUDIOPLAYER_H_

#include <assert.h>
#include <string.h>

#include <android/log.h>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

// for native asset manager
#include <sys/types.h>

#include <ffmpeg_mediaplayer.h>
#include <stdint.h>

static const int BUFFER_COUNT = 2;

static const SLEnvironmentalReverbSettings reverbSettings =
    SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

typedef struct AudioPlayer {
    // engine interfaces
    SLObjectItf engineObject;
    SLEngineItf engineEngine;
    
    // output mix interfaces
    SLObjectItf outputMixObject;
    
    // buffer queue player interfaces
    SLObjectItf bqPlayerObject;
    SLPlayItf bqPlayerPlay;
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
    SLEffectSendItf bqPlayerEffectSend;
    SLMuteSoloItf bqPlayerMuteSolo;
    SLVolumeItf bqPlayerVolume;
    
	void (*bqPlayerCallback) (SLAndroidSimpleBufferQueueItf, void *);
    
    //SDL_Surface     *screen;
    void (*audio_callback) (void *userdata, uint8_t *stream, int len);
} AudioPlayer;

void createEngine(AudioPlayer **ps);
void createBufferQueueAudioPlayer(AudioPlayer **ps, void *state, int numChannels, int samplesPerSec);
void setPlayingAudioPlayer(AudioPlayer **ps, int playstate);
void setVolumeUriAudioPlayer(AudioPlayer **ps, int millibel);
void queueAudioSamples(AudioPlayer **ps, void *state);
int enqueue(AudioPlayer **ps, int16_t *data, int size);
void shutdown(AudioPlayer **ps);

#endif /*AUDIOPLAYER_H_*/
