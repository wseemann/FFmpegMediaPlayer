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

	if(SDL_Init(SDL_INIT_VIDEO)) {
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		exit(1);
	}

	ps = &is;
}

void createScreen(VideoPlayer **ps, void *surface, int width, int height) {
    VideoPlayer *is = *ps;

    // Make a screen to put our video
    is->screen = SDL_CreateWindow(
    		"Screen",
    		SDL_WINDOWPOS_UNDEFINED,
    		SDL_WINDOWPOS_UNDEFINED,
    		width,
    		height,
    		0
    );

    if (!is->screen) {
    	fprintf(stderr, "SDL: could not create window - exiting\n");
    	exit(1);
    }

    is->renderer = SDL_CreateRenderer(is->screen, -1, 0);
    if (!is->renderer) {
    	fprintf(stderr, "SDL: could not create renderer - exiting\n");
    	exit(1);
    }

    // set up YV12 pixel array (12 bits per pixel)
    is->yPlaneSz = width * height;
    is->uvPlaneSz = width * height / 4;

    is->yPlane = (Uint8*)malloc(is->yPlaneSz);
    is->uPlane = (Uint8*)malloc(is->uvPlaneSz);
    is->vPlane = (Uint8*)malloc(is->uvPlaneSz);

    if (!is->yPlane || !is->uPlane || !is->vPlane) {
    	fprintf(stderr, "Could not allocate pixel buffers - exiting\n");
    	exit(1);
    }

    is->uvPitch = width / 2;
}

struct SwsContext *createScaler(VideoPlayer **ps, AVCodecContext *codec) {
	struct SwsContext * sws_ctx;

	sws_ctx = sws_getContext(codec->width,
			codec->height,
			codec->pix_fmt,
			codec->width,
			codec->height,
			AV_PIX_FMT_YUV420P,
			SWS_BILINEAR,
			NULL,
			NULL,
			NULL);

	return sws_ctx;
}

void *createBmp(VideoPlayer **ps, int width, int height) {
	VideoPlayer *is = *ps;

	SDL_Texture *bmp = SDL_CreateTexture(is->renderer, SDL_PIXELFORMAT_YV12,
			SDL_TEXTUREACCESS_STREAMING, width, height);

	return bmp;
}

void destroyBmp(VideoPlayer **ps, void *bmp) {
	SDL_DestroyTexture(bmp);
}

void updateBmp(VideoPlayer **ps, struct SwsContext *sws_ctx, void *bmp, AVFrame *pFrame, int width, int height) {
	VideoPlayer *is = *ps;

	AVPicture pict;
	pict.data[0] = is->yPlane;
	pict.data[1] = is->uPlane;
	pict.data[2] = is->vPlane;
	pict.linesize[0] = width;
	pict.linesize[1] = is->uvPitch;
	pict.linesize[2] = is->uvPitch;

	// Convert the image into YUV format that SDL uses
	sws_scale(sws_ctx, (uint8_t const * const *) pFrame->data,
			pFrame->linesize, 0, height, pict.data,
			pict.linesize);

	SDL_UpdateYUVTexture(
			bmp,
			NULL,
			is->yPlane,
		    width,
			is->uPlane,
			is->uvPitch,
			is->vPlane,
			is->uvPitch);
}

void displayBmp(VideoPlayer **ps, void *bmp) {
	VideoPlayer *is = *ps;

	SDL_RenderClear(is->renderer);
	SDL_RenderCopy(is->renderer, bmp, NULL, NULL);
	SDL_RenderPresent(is->renderer);
}

void shutdownVideoEngine(VideoPlayer **ps) {
	VideoPlayer *is = *ps;

	if (is->renderer) {
		SDL_DestroyRenderer(is->renderer);
		is->renderer = NULL;
	}

	if (is->screen) {
		SDL_DestroyWindow(is->screen);
		is->screen = NULL;
	}
}
