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

#ifndef FFMPEG_MEDIAPLAYER_H_
#define FFMPEG_MEDIAPLAYER_H_

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <pthread.h>

#define AUDIO_DATA_ID 1
#define MAX_AUDIO_FRAME_SIZE 200000;

const int NOT_FROM_THREAD = 0;
const int FROM_THREAD = 1;

typedef enum media_event_type {
    MEDIA_NOP               = 0, // interface test message
    MEDIA_PREPARED          = 1,
    MEDIA_PLAYBACK_COMPLETE = 2,
    MEDIA_BUFFERING_UPDATE  = 3,
    MEDIA_SEEK_COMPLETE     = 4,
    MEDIA_ERROR             = 100,
} media_event_type;

typedef int media_error_type;
const media_error_type MEDIA_ERROR_UNKNOWN = 1;
const media_error_type MEDIA_ERROR_SERVER_DIED = 100;

typedef enum {
    MEDIA_PLAYER_STATE_ERROR        = 0,
    MEDIA_PLAYER_IDLE               = 1 << 0,
    MEDIA_PLAYER_INITIALIZED        = 1 << 1,
    MEDIA_PLAYER_PREPARING          = 1 << 2,
    MEDIA_PLAYER_PREPARED           = 1 << 3,
    MEDIA_PLAYER_STARTED            = 1 << 4,
    MEDIA_PLAYER_PAUSED             = 1 << 5,
    MEDIA_PLAYER_STOPPED            = 1 << 6,
    MEDIA_PLAYER_PLAYBACK_COMPLETE  = 1 << 7
} media_player_states;

typedef struct State {
	AVFormatContext *pFormatCtx;
	int audio_stream;
	int video_stream;
	AVStream *audio_st;
	AVStream *video_st;
	int buffer_size;
	int loop;

	pthread_t decoder_thread;
	int abort_request;
	int paused;
	int last_paused;
	int seek_req;
	int seek_flags;
	int64_t seek_pos;
	int64_t seek_rel;
	int read_pause_return;
	double audio_clock;
	char filename[1024];
	char headers[2048];
	
	void (*notify_callback) (void*, int, int, int, int);
	int (*init_audio_track_callback) (void*, int, int, int);
	void (*write_audio_callback) (void*, int16_t *, int, int);
	void* clazz;
	
	char *allow[0];
	char *block[0];
	
	int             fd;
	int64_t         offset;
} State;

void init(State **ps);
void disconnect(State **ps);
int setNotifyListener(State **ps,  void* clazz, void (*listener) (void*, int, int, int, int));
int setInitAudioTrackListener(State **ps,  void* clazz, int (*listener) (void*, int, int, int));
int setWriteAudioListener(State **ps,  void* clazz, void (*listener) (void*, int16_t *, int, int));
int setDataSourceURI(State **ps, const char *url, const char *headers);
int setDataSourceFD(State **ps, int fd, int64_t offset, int64_t length);
int suspend();
int resume();
int setMetadataFilter(State **ps, char *allow[], char *block[]);
//int getMetadata(bool update_only, bool apply_filter, Parcel *metadata);
int get_metadata(State **ps, const char* key, char** value);
//int setVideoSurface(const sp<Surface>& surface);
int prepareAsync_l(State **ps);
int prepare(State **ps);
int prepareAsync(State **ps);
int start(State **ps);
int stop(State **ps);
int pause(State **ps);
int isPlaying(State **ps);
int getVideoWidth(int *w);
int getVideoHeight(int *h);
int getCurrentPosition(State **ps, int *msec);
int getDuration(State **ps, int *msec);
int seekTo(State **ps, int msec);
int reset(State **ps);
int setLooping(State **ps, int loop);
//void notify(int msg, int ext1, int ext2);

#endif /*FFMPEG_MEDIAPLAYER_H_*/
