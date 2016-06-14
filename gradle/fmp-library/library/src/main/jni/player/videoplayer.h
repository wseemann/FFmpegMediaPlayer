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

#ifndef VIDEOPLAYER_H_
#define VIDEOPLAYER_H_

#include <stdint.h>
#include <stdio.h>

#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include <ffmpeg_mediaplayer.h>

#include <android/native_window_jni.h>

typedef struct VideoPlayer {
	ANativeWindow* native_window;
} VideoPlayer;

void createVideoEngine(VideoPlayer **ps);
void createScreen(VideoPlayer **ps, void *surface, int width, int height);
void setSurface(VideoPlayer **ps, void *surface);
struct SwsContext *createScaler(VideoPlayer **ps, AVCodecContext *codec);
void *createBmp(VideoPlayer **ps, int width, int height);
void destroyBmp(VideoPlayer **ps, void *bmp);
void updateBmp(VideoPlayer **ps, struct SwsContext *sws_ctx, AVCodecContext *pCodecCtx, void *bmp, AVFrame *pFrame, int width, int height);
void displayBmp(VideoPlayer **ps, void *bmp, AVCodecContext *pCodecCtx, int width, int height);
void shutdownVideoEngine(VideoPlayer **ps);

#endif /* VIDEOPLAYER_H_ */
