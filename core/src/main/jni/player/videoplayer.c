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

const int TARGET_IMAGE_FORMAT = AV_PIX_FMT_RGBA; //AV_PIX_FMT_RGB24;
const int TARGET_IMAGE_CODEC = AV_CODEC_ID_PNG;

void createVideoEngine(VideoPlayer **ps) {
	VideoPlayer *is = *ps;
}

void createScreen(VideoPlayer **ps, void *surface, int width, int height) {
	VideoPlayer *is = *ps;
	is->native_window = surface;
}

void setSurface(VideoPlayer **ps, void *surface) {
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

	return malloc(sizeof(Picture));
}

void destroyBmp(VideoPlayer **ps, void *bmp) {
	Picture *picture = (Picture *) bmp;

	if (picture) {
		if (picture->buffer) {
			free(picture->buffer);
			picture->buffer = NULL;
		}

		free(picture);
		picture = NULL;
	}
}

void updateBmp(VideoPlayer **ps, struct SwsContext *sws_ctx, AVCodecContext *pCodecCtx, void *bmp, AVFrame *pFrame, int width, int height) {
    VideoPlayer *is = *ps;
    
    Picture *picture = (Picture *) bmp;
    
    AVFrame *frame;
    
    int got_packet_ptr = 0;
    
    if (width == -1) {
        width = pCodecCtx->width;
    }
    
    if (height == -1) {
        height = pCodecCtx->height;
    }
    
    frame = av_frame_alloc();
    
    if (!frame) {
        goto fail;
    }
    
    // Determine required buffer size and allocate buffer
    int numBytes = avpicture_get_size(TARGET_IMAGE_FORMAT, width, height);
    picture->buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    
    // set the frame parameters
    frame->format = TARGET_IMAGE_FORMAT;
    frame->width = width;
    frame->height = height;
    
    avpicture_fill(((AVPicture *)frame),
                   picture->buffer,
                   TARGET_IMAGE_FORMAT,
                   width,
                   height);
    
    sws_scale(sws_ctx,
              (const uint8_t * const *) pFrame->data,
              pFrame->linesize,
              0,
              height,
              frame->data,
              frame->linesize);
    
    
    picture->linesize = frame->linesize[0];
    
    // TODO is this right?
    fail:
    av_free(frame);
}

void displayBmp(VideoPlayer **ps, void *bmp, AVCodecContext *pCodecCtx, int width, int height) {
	VideoPlayer *is = *ps;

	Picture *picture = (Picture *) bmp;

	if (width == -1) {
		width = pCodecCtx->width;
	}

	if (height == -1) {
		height = pCodecCtx->height;
	}

	if (is->native_window) {
		ANativeWindow_setBuffersGeometry(is->native_window, width, height, WINDOW_FORMAT_RGBA_8888);

		ANativeWindow_Buffer windowBuffer;

		if (ANativeWindow_lock(is->native_window, &windowBuffer, NULL) == 0) {
			int h = 0;

			for (h = 0; h < height; h++)  {
				memcpy(windowBuffer.bits + h * windowBuffer.stride * 4,
						picture->buffer + h * picture->linesize,
						width*4);
			}

			ANativeWindow_unlockAndPost(is->native_window);
		}
	}
}

void shutdownVideoEngine(VideoPlayer **ps) {

}
