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

#include <videoplayer.h>

void createVideoEngine(VideoPlayer **ps) {
	VideoPlayer *is = *ps;
}

void createScreen(VideoPlayer **ps, void *surface, int width, int height) {
	VideoPlayer *is = *ps;
	is->native_window = surface;
}

struct SwsContext *createScaler(VideoPlayer **ps, AVCodecContext *codec) {
	struct SwsContext * sws_ctx;

	sws_ctx = sws_getContext(codec->width,
			codec->height,
			codec->pix_fmt,
			codec->width,
			codec->height,
			AV_PIX_FMT_RGBA,
			SWS_BILINEAR,
			NULL,
			NULL,
			NULL);

	return sws_ctx;
}

void *createBmp(VideoPlayer **ps, int width, int height) {
	VideoPlayer *is = *ps;

	return av_frame_alloc();
}

void destroyBmp(VideoPlayer **ps, void *bmp) {
	av_frame_free(bmp);
}

void updateBmp(VideoPlayer **ps, struct SwsContext *sws_ctx, void *bmp, AVFrame *pFrame, int width, int height) {
	VideoPlayer *is = *ps;

	bmp = pFrame;
}

void displayBmp(VideoPlayer **ps, void *bmp) {
	VideoPlayer *is = *ps;

}

void shutdownVideoEngine(VideoPlayer **ps) {

}
