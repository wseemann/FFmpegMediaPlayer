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

#ifndef PLAYER_H_
#define PLAYER_H_

#include <assert.h>
#include <string.h>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

// for native asset manager
#include <sys/types.h>

#include <player.h>

static const SLEnvironmentalReverbSettings reverbSettings =
    SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

typedef struct Player {
    // engine interfaces
    SLObjectItf engineObject;
    SLEngineItf engineEngine;
    
    // output mix interfaces
    SLObjectItf outputMixObject;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb;
    
    // buffer queue player interfaces
    SLObjectItf bqPlayerObject;
    SLPlayItf bqPlayerPlay;
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
    SLEffectSendItf bqPlayerEffectSend;
    SLMuteSoloItf bqPlayerMuteSolo;
    SLVolumeItf bqPlayerVolume;
    
    // aux effect on the output mix, used by the buffer queue player
    //SLEnvironmentalReverbSettings reverbSettings;
    
    // URI player interfaces
    //SLObjectItf uriPlayerObject;
    //SLPlayItf uriPlayerPlay;
    //SLSeekItf uriPlayerSeek;
    //SLMuteSoloItf uriPlayerMuteSolo;
    //SLVolumeItf uriPlayerVolume;
    
    // file descriptor player interfaces
    //SLObjectItf fdPlayerObject;
    //SLPlayItf fdPlayerPlay;
    //SLSeekItf fdPlayerSeek;
    //SLMuteSoloItf fdPlayerMuteSolo;
    //SLVolumeItf fdPlayerVolume;
    
    // recorder interfaces
    //SLObjectItf recorderObject;
    //SLRecordItf recorderRecord;
    //SLAndroidSimpleBufferQueueItf recorderBufferQueue;
    
    // synthesized sawtooth clip
    //#define SAWTOOTH_FRAMES 8000
    //static short sawtoothBuffer[SAWTOOTH_FRAMES];
    
    // 5 seconds of recorded audio at 16 kHz mono, 16-bit signed little endian
    //#define RECORDER_FRAMES (16000 * 5)
    //static short recorderBuffer[RECORDER_FRAMES];
    //unsigned recorderSize;
    //SLmilliHertz recorderSR;
    
    // pointer and size of the next player buffer to enqueue, and number of remaining buffers
    short *nextBuffer;
    unsigned nextSize;
    int nextCount;
} Player;

void createEngine(Player **ps);
void createBufferQueueAudioPlayer(Player **ps);
void setPlayingAudioPlayer(Player **ps, int isPlaying);
void setChannelMuteUriAudioPlayer(Player **ps, int chan, int mute);
void setChannelSoloUriAudioPlayer(Player **ps, int chan, int solo);
int getNumChannelsUriAudioPlayer(Player **ps);
void setVolumeUriAudioPlayer(Player **ps, int millibel);
void setMuteUriAudioPlayer(Player **ps, int mute);
void enableStereoPositionUriAudioPlayer(Player **ps, int enable);
void setStereoPositionUriAudioPlayer(Player **ps, int permille);
int enableReverb(Player **ps, int enabled);
int selectClip(Player **ps, int which, int count);
int createAudioRecorder(Player **ps);
void shutdown(Player **ps);

#endif /*PLAYER_H_*/
