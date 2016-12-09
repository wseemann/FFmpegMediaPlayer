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

//#include <android/log.h>
#include <ffmpeg_mediaplayer.h>

static int one = 0;
static int two = 0;

void packet_queue_init(PacketQueue *q) {
  memset(q, 0, sizeof(PacketQueue));
  q->initialized = 1;
  q->mutex = SDL_CreateMutex();
  q->cond = SDL_CreateCond();
}
int packet_queue_put(VideoState *is, PacketQueue *q, AVPacket *pkt) {

  AVPacketList *pkt1;
  if(pkt != &is->flush_pkt && av_dup_packet(pkt) < 0) {
    return -1;
  }
  pkt1 = av_malloc(sizeof(AVPacketList));
  if (!pkt1)
    return -1;
  pkt1->pkt = *pkt;
  pkt1->next = NULL;

  SDL_LockMutex(q->mutex);

  if (!q->last_pkt)
    q->first_pkt = pkt1;
  else
    q->last_pkt->next = pkt1;
  q->last_pkt = pkt1;
  q->nb_packets++;
  q->size += pkt1->pkt.size;
  SDL_CondSignal(q->cond);

  SDL_UnlockMutex(q->mutex);
  return 0;
}
static int packet_queue_get(VideoState *is, PacketQueue *q, AVPacket *pkt, int block)
{
  AVPacketList *pkt1;
  int ret;

  SDL_LockMutex(q->mutex);

  for(;;) {

    if(is->quit) {
      ret = -1;
      break;
    }

    pkt1 = q->first_pkt;
    if (pkt1) {
      q->first_pkt = pkt1->next;
      if (!q->first_pkt)
	q->last_pkt = NULL;
      q->nb_packets--;
      q->size -= pkt1->pkt.size;
      *pkt = pkt1->pkt;
      av_free(pkt1);
      ret = 1;
      break;
    } else if (!block) {
      ret = 0;
      break;
    } else {
      SDL_CondWait(q->cond, q->mutex);
    }
  }
  SDL_UnlockMutex(q->mutex);
  return ret;
}
static void packet_queue_flush(PacketQueue *q) {
  AVPacketList *pkt, *pkt1;

  SDL_LockMutex(q->mutex);
  for(pkt = q->first_pkt; pkt != NULL; pkt = pkt1) {
    pkt1 = pkt->next;
    av_packet_unref(&pkt->pkt);
    av_freep(&pkt);
  }
  q->last_pkt = NULL;
  q->first_pkt = NULL;
  q->nb_packets = 0;
  q->size = 0;
  SDL_UnlockMutex(q->mutex);
}
double get_audio_clock(VideoState *is) {
  double pts;
  int hw_buf_size, bytes_per_sec, n;

  pts = is->audio_clock; /* maintained in the audio thread */
  hw_buf_size = is->audio_buf_size - is->audio_buf_index;
  bytes_per_sec = 0;
  n = is->audio_st->codec->channels * 2;
  if(is->audio_st) {
    bytes_per_sec = is->audio_st->codec->sample_rate * n;
  }
  if(bytes_per_sec) {
    pts -= (double)hw_buf_size / bytes_per_sec;
  }
  return pts;
}
double get_video_clock(VideoState *is) {
  double delta;

  delta = (av_gettime() - is->video_current_pts_time) / 1000000.0;
  return is->video_current_pts + delta;
}
double get_external_clock(VideoState *is) {
  return av_gettime() / 1000000.0;
}
double get_master_clock(VideoState *is) {
  if(is->av_sync_type == AV_SYNC_VIDEO_MASTER) {
    return get_video_clock(is);
  } else if(is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
    return get_audio_clock(is);
  } else {
    return get_external_clock(is);
  }
}
/* Add or subtract samples to get a better sync, return new
   audio buffer size */
int synchronize_audio(VideoState *is, short *samples,
		      int samples_size, double pts) {
  int n;
  double ref_clock;

  n = 2 * is->audio_st->codec->channels;

  if(is->av_sync_type != AV_SYNC_AUDIO_MASTER) {
    double diff, avg_diff;
    int wanted_size, min_size, max_size /*, nb_samples */;

    ref_clock = get_master_clock(is);
    diff = get_audio_clock(is) - ref_clock;

    if(diff < AV_NOSYNC_THRESHOLD) {
      // accumulate the diffs
      is->audio_diff_cum = diff + is->audio_diff_avg_coef
	* is->audio_diff_cum;
      if(is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
	is->audio_diff_avg_count++;
      } else {
	avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);
	if(fabs(avg_diff) >= is->audio_diff_threshold) {
	  wanted_size = samples_size + ((int)(diff * is->audio_st->codec->sample_rate) * n);
	  min_size = samples_size * ((100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100);
	  max_size = samples_size * ((100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100);
	  if(wanted_size < min_size) {
	    wanted_size = min_size;
	  } else if (wanted_size > max_size) {
	    wanted_size = max_size;
	  }
	  if(wanted_size < samples_size) {
	    /* remove samples */
	    samples_size = wanted_size;
	  } else if(wanted_size > samples_size) {
	    uint8_t *samples_end, *q;
	    int nb;

	    /* add samples by copying final sample*/
	    nb = (samples_size - wanted_size);
	    samples_end = (uint8_t *)samples + samples_size - n;
	    q = samples_end + n;
	    while(nb > 0) {
	      memcpy(q, samples_end, n);
	      q += n;
	      nb -= n;
	    }
	    samples_size = wanted_size;
	  }
	}
      }
    } else {
      /* difference is TOO big; reset diff stuff */
      is->audio_diff_avg_count = 0;
      is->audio_diff_cum = 0;
    }
  }
  return samples_size;
}

int decode_frame_from_packet(VideoState *is, AVFrame decoded_frame)
{
	int64_t src_ch_layout, dst_ch_layout;
	int src_rate, dst_rate;
	uint8_t **src_data = NULL, **dst_data = NULL;
	int src_nb_channels = 0, dst_nb_channels = 0;
	int src_linesize, dst_linesize;
	int src_nb_samples, dst_nb_samples, max_dst_nb_samples;
	enum AVSampleFormat src_sample_fmt, dst_sample_fmt;
	int dst_bufsize;
	int ret;

	src_nb_samples = decoded_frame.nb_samples;
	src_linesize = (int) decoded_frame.linesize;
	src_data = decoded_frame.data;

	if (decoded_frame.channel_layout == 0) {
		decoded_frame.channel_layout = av_get_default_channel_layout(decoded_frame.channels);
	}

	src_rate = decoded_frame.sample_rate;
	dst_rate = decoded_frame.sample_rate;
	src_ch_layout = decoded_frame.channel_layout;
	dst_ch_layout = decoded_frame.channel_layout;
	src_sample_fmt = decoded_frame.format;
	dst_sample_fmt = AV_SAMPLE_FMT_S16;

	/* allocate source and destination samples buffers */
	src_nb_channels = av_get_channel_layout_nb_channels(src_ch_layout);
	ret = av_samples_alloc_array_and_samples(&src_data, &src_linesize, src_nb_channels, src_nb_samples, src_sample_fmt, 0);
	if (ret < 0) {
		fprintf(stderr, "Could not allocate source samples\n");
		return -1;
	}

	/* compute the number of converted samples: buffering is avoided
	 * ensuring that the output buffer will contain at least all the
	 * converted input samples */
	max_dst_nb_samples = dst_nb_samples = av_rescale_rnd(src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);

	/* buffer is going to be directly written to a rawaudio file, no alignment */
	dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);
	ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_nb_channels, dst_nb_samples, dst_sample_fmt, 0);
	if (ret < 0) {
		fprintf(stderr, "Could not allocate destination samples\n");
		return -1;
	}

	/* compute destination number of samples */
	dst_nb_samples = av_rescale_rnd(swr_get_delay(is->sws_ctx_audio, src_rate) + src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);

	/* convert to destination format */
	ret = swr_convert(is->sws_ctx_audio, dst_data, dst_nb_samples, (const uint8_t **)decoded_frame.data, src_nb_samples);
	if (ret < 0) {
		fprintf(stderr, "Error while converting\n");
		return -1;
	}

	dst_bufsize = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels, ret, dst_sample_fmt, 1);
	if (dst_bufsize < 0) {
		fprintf(stderr, "Could not get sample buffer size\n");
		return -1;
	}

	memcpy(is->audio_buf, dst_data[0], dst_bufsize);

	if (src_data) {
		av_freep(&src_data[0]);
	}
	av_freep(&src_data);

	if (dst_data) {
		av_freep(&dst_data[0]);
	}
	av_freep(&dst_data);

	return dst_bufsize;
}

int audio_decode_frame(VideoState *is, double *pts_ptr) {

  int len1, data_size = 0, n;
  AVPacket *pkt = &is->audio_pkt;
  double pts;

  for(;;) {
    while(is->audio_pkt_size > 0) {
      int got_frame = 0;
      len1 = avcodec_decode_audio4(is->audio_st->codec, &is->audio_frame, &got_frame, pkt);
      if(len1 < 0) {
	/* if error, skip frame */
	is->audio_pkt_size = 0;
	break;
      }
      if (got_frame)
      {
    	  if (is->audio_frame.format != AV_SAMPLE_FMT_S16) {
    		  data_size = decode_frame_from_packet(is, is->audio_frame);
    	  } else {
            data_size =
              av_samples_get_buffer_size
              (
                  NULL,
                  is->audio_st->codec->channels,
                  is->audio_frame.nb_samples,
                  is->audio_st->codec->sample_fmt,
                  1
              );
            memcpy(is->audio_buf, is->audio_frame.data[0], data_size);
    	  }
      }
      is->audio_pkt_data += len1;
      is->audio_pkt_size -= len1;
      if(data_size <= 0) {
	/* No data yet, get more frames */
	continue;
      }
      pts = is->audio_clock;
      *pts_ptr = pts;
      n = 2 * is->audio_st->codec->channels;
      is->audio_clock += (double)data_size /
	(double)(n * is->audio_st->codec->sample_rate);

      /* We have data, return it and come back for more later */
      return data_size;
    }
    if(pkt->data)
      av_packet_unref(pkt);

    if(is->quit) {
      return -1;
    }
    /* next packet */
    if(packet_queue_get(is, &is->audioq, pkt, 1) < 0) {
      return -1;
    }
    if(pkt->data == is->flush_pkt.data) {
      avcodec_flush_buffers(is->audio_st->codec);
      continue;
    }
    is->audio_pkt_data = pkt->data;
    is->audio_pkt_size = pkt->size;
    /* if update, update the audio clock w/pts */
    if(pkt->pts != AV_NOPTS_VALUE) {
      is->audio_clock = av_q2d(is->audio_st->time_base)*pkt->pts;
    }
  }
}

void audio_callback(void *userdata, Uint8 *stream, int len) {

  VideoState *is = (VideoState *)userdata;
  int len1, audio_size;
  double pts;

  while(len > 0) {
    if(is->audio_buf_index >= is->audio_buf_size) {
      /* We have already sent all our data; get more */
      audio_size = audio_decode_frame(is, &pts);
      if(audio_size < 0) {
	/* If error, output silence */
	is->audio_buf_size = 1024;
	memset(is->audio_buf, 0, is->audio_buf_size);
      } else {
	audio_size = synchronize_audio(is, (int16_t *)is->audio_buf,
				       audio_size, pts);
	is->audio_buf_size = audio_size;
      }
      is->audio_buf_index = 0;
    }
    len1 = is->audio_buf_size - is->audio_buf_index;
    if(len1 > len)
      len1 = len;
    memcpy(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1);
    len -= len1;
    stream += len1;
    is->audio_buf_index += len1;
  }

  //notify_from_thread(is, MEDIA_BUFFERING_UPDATE, 0, 0);
}

void video_refresh_timer(void *userdata);

static Uint32 sdl_refresh_timer_cb(Uint32 interval, void *opaque) {
  video_refresh_timer(opaque);
  return 0; /* 0 means stop timer */
}

/* schedule a video refresh in 'delay' ms */
static void schedule_refresh(VideoState *is, int delay) {
  SDL_AddTimer(delay, sdl_refresh_timer_cb, is);
}

void video_display(VideoState *is) {

  SDL_Rect rect;
  VideoPicture *vp;
  //AVPicture pict;
  float aspect_ratio;
  int w, h, x, y;
  //int i;

  vp = &is->pictq[is->pictq_rindex];
  if(vp->bmp) {
    if(is->video_st->codec->sample_aspect_ratio.num == 0) {
      aspect_ratio = 0;
    } else {
      aspect_ratio = av_q2d(is->video_st->codec->sample_aspect_ratio) *
	is->video_st->codec->width / is->video_st->codec->height;
    }
    if(aspect_ratio <= 0.0) {
      aspect_ratio = (float)is->video_st->codec->width /
	(float)is->video_st->codec->height;
    }
    /*h = screen->h;
    w = ((int)rint(h * aspect_ratio)) & -3;
    if(w > screen->w) {
      w = screen->w;
      h = ((int)rint(w / aspect_ratio)) & -3;
    }
    x = (screen->w - w) / 2;
    y = (screen->h - h) / 2;

    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    SDL_DisplayYUVOverlay(vp->bmp, &rect);*/

    displayBmp(&is->video_player, vp->bmp, is->video_st->codec, is->video_st->codec->width, is->video_st->codec->height);
    free(vp->bmp->buffer);
  }
}

//void video_refresh_timer(void *userdata) {
//
//	VideoState *is = (VideoState *)userdata;
//	VideoPicture *vp;
//	double actual_delay, delay, sync_threshold, ref_clock, diff;
//
//	if(is->video_st) {
//		if(is->pictq_size == 0) {
//			schedule_refresh(is, 1);
//		} else {
//			vp = &is->pictq[is->pictq_rindex];
//
//			is->video_current_pts = vp->pts;
//			is->video_current_pts_time = av_gettime();
//
//			delay = vp->pts - is->frame_last_pts; /* the pts from last time */
//			if(delay <= 0 || delay >= 1.0) {
//				/* if incorrect delay, use previous one */
//				delay = is->frame_last_delay;
//			}
//			/* save for next time */
//			is->frame_last_delay = delay;
//			is->frame_last_pts = vp->pts;
//
//			/* update delay to sync to audio if not master source */
//			if(is->av_sync_type != AV_SYNC_VIDEO_MASTER) {
//				ref_clock = get_master_clock(is);
//				diff = vp->pts - ref_clock;
//
//				/* Skip or repeat the frame. Take delay into account
//	   FFPlay still doesn't "know if this is the best guess." */
//				sync_threshold = (delay > AV_SYNC_THRESHOLD) ? delay : AV_SYNC_THRESHOLD;
//				if(fabs(diff) < AV_NOSYNC_THRESHOLD) {
//					if(diff <= -sync_threshold) {
//						delay = 0;
//					} else if(diff >= sync_threshold) {
//						delay = 2 * delay;
//					}
//				}
//			}
//
//			is->frame_timer += delay;
//			/* computer the REAL delay */
//			actual_delay = is->frame_timer - (av_gettime() / 1000000.0);
//			if(actual_delay < 0.010) {
//				/* Really it should skip the picture instead */
//				actual_delay = 0.010;
//			}
//			schedule_refresh(is, (int)(actual_delay * 1000 + 0.5));
//
//			/* show the picture! */
//			video_display(is);
//
//			/* update queue for next picture! */
//			if(++is->pictq_rindex == VIDEO_PICTURE_QUEUE_SIZE) {
//				is->pictq_rindex = 0;
//			}
//			SDL_LockMutex(is->pictq_mutex);
//			is->pictq_size--;
//			SDL_CondSignal(is->pictq_cond);
//			SDL_UnlockMutex(is->pictq_mutex);
//		}
//	} else {
//		schedule_refresh(is, 100);
//	}
//}

void video_refresh_timer(void *opaque) {
	VideoState *is = (VideoState *)opaque;

	VideoPicture *vp;
	double actual_delay, delay, sync_threshold, ref_clock, diff;

    for(;;) {
	    if(is->quit) {
	        break;
	    }

	    if(is->video_st) {
	        if(is->pictq_size == 0) {
	          //schedule_refresh(is, 1);
	          SDL_Delay(1);
	          continue;
	        } else {
	          vp = &is->pictq[is->pictq_rindex];

	          is->video_current_pts = vp->pts;
	          is->video_current_pts_time = av_gettime();

	          delay = vp->pts - is->frame_last_pts; /* the pts from last time */
	          if(delay <= 0 || delay >= 1.0) {
	    	/* if incorrect delay, use previous one */
	    	delay = is->frame_last_delay;
	          }
	          /* save for next time */
	          is->frame_last_delay = delay;
	          is->frame_last_pts = vp->pts;

	          /* update delay to sync to audio if not master source */
	          if(is->av_sync_type != AV_SYNC_VIDEO_MASTER) {
	    	ref_clock = get_master_clock(is);
	    	diff = vp->pts - ref_clock;

	    	/* Skip or repeat the frame. Take delay into account
	    	   FFPlay still doesn't "know if this is the best guess." */
	    	sync_threshold = (delay > AV_SYNC_THRESHOLD) ? delay : AV_SYNC_THRESHOLD;
	    	if(fabs(diff) < AV_NOSYNC_THRESHOLD) {
	    	  if(diff <= -sync_threshold) {
	    	    delay = 0;
	    	  } else if(diff >= sync_threshold) {
	    	    delay = 2 * delay;
	    	  }
	    	}
	          }

	          is->frame_timer += delay;
	          /* computer the REAL delay */
	          actual_delay = is->frame_timer - (av_gettime() / 1000000.0);
	          if(actual_delay < 0.010) {
	    	/* Really it should skip the picture instead */
	    	actual_delay = 0.010;
	          }
	          //schedule_refresh(is, (int)(actual_delay * 1000 + 0.5));

	          /* show the picture! */
	          video_display(is);

	          /* update queue for next picture! */
	          if(++is->pictq_rindex == VIDEO_PICTURE_QUEUE_SIZE) {
	    	is->pictq_rindex = 0;
	          }
	          SDL_LockMutex(is->pictq_mutex);
	          is->pictq_size--;
	          SDL_CondSignal(is->pictq_cond);
	          SDL_UnlockMutex(is->pictq_mutex);

	          SDL_Delay((int)(actual_delay * 1000 + 0.5));
	          continue;
	        }
	      } else {
	        //schedule_refresh(is, 100);
	        SDL_Delay(100);
	        continue;
	      }
    }
}

void alloc_picture(void *userdata) {

  VideoState *is = (VideoState *)userdata;
  VideoPicture *vp;

  vp = &is->pictq[is->pictq_windex];
  if(vp->bmp) {
    // we already have one make another, bigger/smaller
	destroyBmp(&is->video_player, vp->bmp);
  }
  // Allocate a place to put our YUV image on that screen
  vp->bmp = createBmp(&is->video_player, is->video_st->codec->width, is->video_st->codec->height);

  vp->width = is->video_st->codec->width;
  vp->height = is->video_st->codec->height;

  SDL_LockMutex(is->pictq_mutex);
  vp->allocated = 1;
  SDL_CondSignal(is->pictq_cond);
  SDL_UnlockMutex(is->pictq_mutex);

}

int queue_picture(VideoState *is, AVFrame *pFrame, double pts) {

  VideoPicture *vp;
  //int dst_pix_fmt;
  AVPicture pict;

  /* wait until we have space for a new pic */
  SDL_LockMutex(is->pictq_mutex);
  while(is->pictq_size >= VIDEO_PICTURE_QUEUE_SIZE &&
	!is->quit) {
    SDL_CondWait(is->pictq_cond, is->pictq_mutex);
  }
  SDL_UnlockMutex(is->pictq_mutex);

  if(is->quit)
    return -1;

  // windex is set to 0 initially
  vp = &is->pictq[is->pictq_windex];

  /* allocate or resize the buffer! */
  if(!vp->bmp ||
     vp->width != is->video_st->codec->width ||
     vp->height != is->video_st->codec->height) {
    //SDL_Event event;

    vp->allocated = 0;
    /* we have to do it in the main thread */
    //event.type = FF_ALLOC_EVENT;
    //event.user.data1 = is;
    //SDL_PushEvent(&event);
    alloc_picture(is);

    /* wait until we have a picture allocated */
    SDL_LockMutex(is->pictq_mutex);
    while(!vp->allocated && !is->quit) {
      SDL_CondWait(is->pictq_cond, is->pictq_mutex);
    }
    SDL_UnlockMutex(is->pictq_mutex);
    if(is->quit) {
      return -1;
    }
  }
  /* We have a place to put our picture on the queue */
  /* If we are skipping a frame, do we set this to null
     but still return vp->allocated = 1? */


  if(vp->bmp) {

    //dst_pix_fmt = PIX_FMT_YUV420P;
    /* point pict at the queue */

    updateBmp(&is->video_player, is->sws_ctx, is->video_st->codec, vp->bmp, pFrame, is->video_st->codec->width, is->video_st->codec->height);

    vp->pts = pts;

    /* now we inform our display thread that we have a pic ready */
    if(++is->pictq_windex == VIDEO_PICTURE_QUEUE_SIZE) {
      is->pictq_windex = 0;
    }
    SDL_LockMutex(is->pictq_mutex);
    is->pictq_size++;
    SDL_UnlockMutex(is->pictq_mutex);
  }
  return 0;
}

double synchronize_video(VideoState *is, AVFrame *src_frame, double pts) {

  double frame_delay;

  if(pts != 0) {
    /* if we have pts, set video clock to it */
    is->video_clock = pts;
  } else {
    /* if we aren't given a pts, set it to the clock */
    pts = is->video_clock;
  }
  /* update the video clock */
  frame_delay = av_q2d(is->video_st->codec->time_base);
  /* if we are repeating a frame, adjust clock accordingly */
  frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
  is->video_clock += frame_delay;
  return pts;
}

uint64_t global_video_pkt_pts = AV_NOPTS_VALUE;

/* These are called whenever we allocate a frame
 * buffer. We use this to store the global_pts in
 * a frame at the time it is allocated.
 */
int our_get_buffer(struct AVCodecContext *c, AVFrame *pic, int flags) {
  int ret = avcodec_default_get_buffer2 (c, pic, flags);
  uint64_t *pts = av_malloc(sizeof(uint64_t));
  *pts = global_video_pkt_pts;
  pic->opaque = pts;
  return ret;
}

int video_thread(void *arg) {
  VideoState *is = (VideoState *)arg;
  AVPacket pkt1, *packet = &pkt1;
  int frameFinished;
  AVFrame *pFrame;
  double pts;

  pFrame = av_frame_alloc();

  for(;;) {
    if(packet_queue_get(is, &is->videoq, packet, 1) < 0) {
      // means we quit getting packets
      break;
    }
    if(packet->data == is->flush_pkt.data) {
      avcodec_flush_buffers(is->video_st->codec);
      continue;
    }
    pts = 0;

    // Save global pts to be stored in pFrame in first call
    global_video_pkt_pts = packet->pts;
    // Decode video frame
    avcodec_decode_video2(is->video_st->codec, pFrame, &frameFinished,
				packet);
    if(packet->dts == AV_NOPTS_VALUE
       && pFrame->opaque && *(uint64_t*)pFrame->opaque != AV_NOPTS_VALUE) {
      pts = *(uint64_t *)pFrame->opaque;
    } else if(packet->dts != AV_NOPTS_VALUE) {
      pts = packet->dts;
    } else {
      pts = 0;
    }
    pts *= av_q2d(is->video_st->time_base);

    // Did we get a video frame?
    if(frameFinished) {
      pts = synchronize_video(is, pFrame, pts);
      if(queue_picture(is, pFrame, pts) < 0) {
	break;
      }
    }
    av_packet_unref(packet);
  }
  av_free(pFrame);

  two = 1;
  return 0;
}
int stream_component_open(VideoState *is, int stream_index) {

  AVFormatContext *pFormatCtx = is->pFormatCtx;
  AVCodecContext *codecCtx = NULL;
  AVCodec *codec = NULL;
  AVDictionary *optionsDict = NULL;

  if(stream_index < 0 || stream_index >= pFormatCtx->nb_streams) {
    return -1;
  }

  // Get a pointer to the codec context for the video stream
  codecCtx = pFormatCtx->streams[stream_index]->codec;

  if(codecCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
	is->audio_callback = audio_callback;

    // Set audio settings from codec info
	AudioPlayer *player = malloc(sizeof(AudioPlayer));
    is->audio_player = player;
    createEngine(&is->audio_player);
    createBufferQueueAudioPlayer(&is->audio_player, is, codecCtx->channels, codecCtx->sample_rate);
    //is->audio_hw_buf_size = 4096;
  } else if (codecCtx->codec_type == AVMEDIA_TYPE_VIDEO) {
	// Set video settings from codec info
	VideoPlayer *player = malloc(sizeof(VideoPlayer));
	is->video_player = player;
	createVideoEngine(&is->video_player);
	createScreen(&is->video_player, is->native_window, 0, 0);
  }
  codec = avcodec_find_decoder(codecCtx->codec_id);
  if(!codec || (avcodec_open2(codecCtx, codec, &optionsDict) < 0)) {
    fprintf(stderr, "Unsupported codec!\n");
    return -1;
  }

  switch(codecCtx->codec_type) {
  case AVMEDIA_TYPE_AUDIO:
    is->audioStream = stream_index;
    is->audio_st = pFormatCtx->streams[stream_index];
    is->audio_buf_size = 0;
    is->audio_buf_index = 0;

    /* averaging filter for audio sync */
    is->audio_diff_avg_coef = exp(log(0.01 / AUDIO_DIFF_AVG_NB));
    is->audio_diff_avg_count = 0;
    /* Correct audio only if larger error than this */
    is->audio_diff_threshold = 2.0 * SDL_AUDIO_BUFFER_SIZE / codecCtx->sample_rate;

	is->sws_ctx_audio = swr_alloc();
	if (!is->sws_ctx_audio) {
		fprintf(stderr, "Could not allocate resampler context\n");
		return -1;
	}

	uint64_t channel_layout = is->audio_st->codec->channel_layout;

	if (channel_layout == 0) {
		channel_layout = av_get_default_channel_layout(is->audio_st->codec->channels);
	}

	av_opt_set_int(is->sws_ctx_audio, "in_channel_layout", channel_layout, 0);
	av_opt_set_int(is->sws_ctx_audio, "out_channel_layout", channel_layout,  0);
	av_opt_set_int(is->sws_ctx_audio, "in_sample_rate", is->audio_st->codec->sample_rate, 0);
	av_opt_set_int(is->sws_ctx_audio, "out_sample_rate", is->audio_st->codec->sample_rate, 0);
	av_opt_set_sample_fmt(is->sws_ctx_audio, "in_sample_fmt", is->audio_st->codec->sample_fmt, 0);
	av_opt_set_sample_fmt(is->sws_ctx_audio, "out_sample_fmt", AV_SAMPLE_FMT_S16,  0);

	/* initialize the resampling context */
	if ((swr_init(is->sws_ctx_audio)) < 0) {
		fprintf(stderr, "Failed to initialize the resampling context\n");
		return -1;
	}

    memset(&is->audio_pkt, 0, sizeof(is->audio_pkt));
    packet_queue_init(&is->audioq);
    break;
  case AVMEDIA_TYPE_VIDEO:
    is->videoStream = stream_index;
    is->video_st = pFormatCtx->streams[stream_index];

    is->frame_timer = (double)av_gettime() / 1000000.0;
    is->frame_last_delay = 40e-3;
    is->video_current_pts_time = av_gettime();

    packet_queue_init(&is->videoq);

    createScreen(&is->video_player, is->native_window, is->video_st->codec->width, is->video_st->codec->height);

    is->video_tid = malloc(sizeof(*(is->video_tid)));

    pthread_create(is->video_tid, NULL, (void *) &video_thread, is);
    is->sws_ctx = createScaler(&is->video_player, is->video_st->codec);

    codecCtx->get_buffer2 = our_get_buffer;

    break;
  default:
    break;
  }

  return 0;
}

int decode_interrupt_cb(void *opaque) {
  VideoState *is = (VideoState *)opaque;

  return (is && is->quit);
}
int decode_thread(void *arg) {

  VideoState *is = (VideoState *)arg;
  AVPacket pkt1, *packet = &pkt1;

  AVDictionary *io_dict = NULL;
  AVIOInterruptCB callback;

  int video_index = -1;
  int audio_index = -1;
  int i;

  int ret;
  int eof = 0;

  is->videoStream=-1;
  is->audioStream=-1;

  AVDictionary *options = NULL;
  av_dict_set(&options, "icy", "1", 0);
  av_dict_set(&options, "user-agent", "FFmpegMediaPlayer", 0);
    
  if (is->headers) {
    av_dict_set(&options, "headers", is->headers, 0);
  }

  if (is->offset > 0) {
    is->pFormatCtx = avformat_alloc_context();
    is->pFormatCtx->skip_initial_bytes = is->offset;
    //is->pFormatCtx->iformat = av_find_input_format("mp3");
  }

  // will interrupt blocking functions if we quit!
  callback.callback = decode_interrupt_cb;
  callback.opaque = is;
  if (avio_open2(&is->io_context, is->filename, 0, &callback, &io_dict))
  {
    fprintf(stderr, "Unable to open I/O for %s\n", is->filename);
    notify_from_thread(is, MEDIA_ERROR, 0, 0);
    return -1;
  }

  // Open video file
  if(avformat_open_input(&is->pFormatCtx, is->filename, NULL, &options)!=0)
  {
	  notify_from_thread(is, MEDIA_ERROR, 0, 0);
    return -1; // Couldn't open file
  }

  // Retrieve stream information
  if(avformat_find_stream_info(is->pFormatCtx, NULL)<0)
  {
	  notify_from_thread(is, MEDIA_ERROR, 0, 0);
    return -1; // Couldn't find stream information
  }

  // Dump information about file onto standard error
  av_dump_format(is->pFormatCtx, 0, is->filename, 0);

  // Find the first video stream
  for(i=0; i<is->pFormatCtx->nb_streams; i++) {
    if(is->pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO &&
       video_index < 0) {
      video_index=i;
    }
    if(is->pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO &&
       audio_index < 0) {
      audio_index=i;
    }
      
    set_codec(is->pFormatCtx, i);
  }
  if(audio_index >= 0) {
    stream_component_open(is, audio_index);
  }
  if(video_index >= 0) {
    stream_component_open(is, video_index);
  }

  if(is->videoStream < 0 && is->audioStream < 0) {
  //if(is->videoStream < 0 || is->audioStream < 0) {
    fprintf(stderr, "%s: could not open codecs\n", is->filename);
    notify_from_thread(is, MEDIA_ERROR, 0, 0);
    return 0;
  }

  set_rotation(is->pFormatCtx, is->audio_st, is->video_st);
  set_framerate(is->pFormatCtx, is->audio_st, is->video_st);
  set_filesize(is->pFormatCtx);
  set_chapter_count(is->pFormatCtx);
  //set_video_dimensions(is->pFormatCtx, is->video_st);

  notify_from_thread(is, MEDIA_INFO, MEDIA_INFO_METADATA_UPDATE, 0);

  // main decode loop

  for(;;) {
    if(is->quit) {
      break;
    }

    /*if (is->paused != is->last_paused) {
        is->last_paused = is->paused;
        if (is->paused)
            is->read_pause_return = av_read_pause(is->pFormatCtx);
        else
            av_read_play(is->pFormatCtx);
    }*/

    // seek stuff goes here
    if(is->seek_req) {
		int64_t seek_target = is->seek_pos;
		int64_t seek_min    = is->seek_rel > 0 ? seek_target - is->seek_rel + 2: INT64_MIN;
		int64_t seek_max    = is->seek_rel < 0 ? seek_target - is->seek_rel - 2: INT64_MAX;

		int ret = avformat_seek_file(is->pFormatCtx, -1, seek_min, seek_target, seek_max, is->seek_flags);
      if(ret < 0) {
	fprintf(stderr, "%s: error while seeking\n", is->pFormatCtx->filename);
      } else {
	if(is->audioStream >= 0) {
	  packet_queue_flush(&is->audioq);
	  packet_queue_put(is, &is->audioq, &is->flush_pkt);
	}
	if(is->videoStream >= 0) {
	  packet_queue_flush(&is->videoq);
	  packet_queue_put(is, &is->videoq, &is->flush_pkt);
	}
	notify_from_thread(is, MEDIA_SEEK_COMPLETE, 0, 0);

      }
      is->seek_req = 0;
      eof = 0;
    }

    if (is->audioq.size >= MAX_AUDIOQ_SIZE && !is->prepared) {
        queueAudioSamples(&is->audio_player, is);

        notify_from_thread(is, MEDIA_PREPARED, 0, 0);
    	is->prepared = 1;
    }

    if(is->audioq.size > MAX_AUDIOQ_SIZE ||
       is->videoq.size > MAX_VIDEOQ_SIZE) {
      SDL_Delay(10);
      continue;
    }
    if((ret = av_read_frame(is->pFormatCtx, packet)) < 0) {
      if (ret == AVERROR_EOF || !is->pFormatCtx->pb->eof_reached) {
          eof = 1;
    	  break;
      }

      if(is->pFormatCtx->pb->error == 0) {
	SDL_Delay(100); /* no error; wait for user input */
	continue;
      } else {
	break;
      }
    }
    // Is this a packet from the video stream?
    if(packet->stream_index == is->videoStream) {
      packet_queue_put(is, &is->videoq, packet);
    } else if(packet->stream_index == is->audioStream) {
      packet_queue_put(is, &is->audioq, packet);
    } else {
      av_packet_unref(packet);
    }

	if (eof) {
		break;
	}
  }

  if (eof) {
	  notify_from_thread(is, MEDIA_PLAYBACK_COMPLETE, 0, 0);
  }

  one = 1;
  return 0;
}

void stream_seek(VideoState *is, int64_t pos, int64_t rel, int seek_by_bytes) {
	if (!is->seek_req) {
		is->seek_pos = pos;
		is->seek_rel = rel;
		is->seek_flags &= ~AVSEEK_FLAG_BYTE;
		if (seek_by_bytes)
			is->seek_flags |= AVSEEK_FLAG_BYTE;
		is->seek_req = 1;
	}
}

VideoState * create() {
	VideoState *is;

	is = av_mallocz(sizeof(VideoState));
	is->last_paused = -1;

    return is;
}

VideoState * getNextMediaPlayer(VideoState **ps) {
	/*VideoState *is = *ps;
    
    if (is && is->next) {
        return is->next;
    }*/
    
    return NULL;
}

void disconnect(VideoState **ps) {
	VideoState *is = *ps;

	if (is) {
		if (is->pFormatCtx) {
			avformat_close_input(&is->pFormatCtx);
			is->pFormatCtx = NULL;
		}

		if (is->audioq.initialized == 1) {
			if (is->audioq.first_pkt) {
				free(is->audioq.first_pkt);
			}

			if (is->audioq.last_pkt) {
				//free(is->audioq.last_pkt);
			}

			if (is->audioq.mutex) {
				free(is->audioq.mutex);
				is->audioq.mutex = NULL;
			}

			if (is->audioq.cond) {
				free(is->audioq.cond);
				is->audioq.cond = NULL;
			}

			is->audioq.initialized = 0;
		}

		/*AVFrame *frame = &is->audio_frame;
		    if (frame->data) {
		    	av_free_frame(frame);
		    }*/

		AVPacket *pkt = &is->audio_pkt;
		if (pkt->data) {
			av_packet_unref(pkt);
		}

		if (is->videoq.initialized == 1) {
			if (is->videoq.first_pkt) {
				free(is->videoq.first_pkt);
			}

			if (is->videoq.last_pkt) {
				//free(is->videoq.last_pkt);
			}

			if (is->videoq.mutex) {
				free(is->videoq.mutex);
				is->videoq.mutex = NULL;
			}

			if (is->videoq.cond) {
				free(is->videoq.cond);
				is->videoq.cond = NULL;
			}

			is->videoq.initialized = 0;
		}

		//VideoPicture    pictq[VIDEO_PICTURE_QUEUE_SIZE];

		if (is->pictq_mutex) {
			free(is->pictq_mutex);
			is->pictq_mutex = NULL;
		}

		if (is->pictq_cond) {
			free(is->pictq_cond);
			is->pictq_cond = NULL;
		}

		if (is->parse_tid) {
			free(is->parse_tid);
			is->parse_tid = NULL;
		}

		if (is->video_tid) {
			free(is->video_tid);
			is->video_tid = NULL;
		}

		if (is->io_context) {
			avio_close(is->io_context);
			is->io_context = NULL;
		}

		if (is->sws_ctx) {
			sws_freeContext(is->sws_ctx);
			is->sws_ctx = NULL;
		}

		if (is->sws_ctx_audio) {
			swr_free(&is->sws_ctx_audio);
			is->sws_ctx_audio = NULL;
		}

		if (is->audio_player) {
			shutdown(&is->audio_player);
			is->audio_player = NULL;
		}

		if (is->tid) {
			free(is->tid);
			is->tid = NULL;
		}

	    av_packet_unref(&is->flush_pkt);

		av_freep(&is);
		*ps = NULL;
	}
}

int setDataSourceURI(VideoState **ps, const char *url, const char *headers) {
	printf("setDataSource\n");

	if (!url) {
		return INVALID_OPERATION;
	}

	VideoState *is = *ps;

	// Workaround for FFmpeg ticket #998
	// "must convert mms://... streams to mmsh://... for FFmpeg to work"
	char *restrict_to = strstr(url, "mms://");
	if (restrict_to) {
		strncpy(restrict_to, "mmsh://", 6);
		puts(url);
	}

	strncpy(is->filename, url, sizeof(is->filename));

	if (headers) {
		strncpy(is->headers, headers, sizeof(is->headers));
	}

	return NO_ERROR;
}

int setDataSourceFD(VideoState **ps, int fd, int64_t offset, int64_t length) {
	printf("setDataSource\n");

	VideoState *is = *ps;

	int myfd = dup(fd);

	char str[20];
	sprintf(str, "pipe:%d", myfd);
	strncpy(is->filename, str, sizeof(is->filename));

	is->fd = myfd;
	is->offset = offset;

	*ps = is;

	return NO_ERROR;
}

int setVideoSurface(VideoState **ps, ANativeWindow* native_window) {
    printf("set_native_window\n");

	VideoState *is = *ps;

	is->native_window = native_window;

	if (is && is->video_player) {
		setSurface(&is->video_player, is->native_window);
	}

	*ps = is;

	return NO_ERROR;
}

int setListener(VideoState **ps, void* clazz, void (*listener) (void*, int, int, int, int)) {
	VideoState *is = *ps;

	is->clazz = clazz;
	is->notify_callback = listener;

	return NO_ERROR;
}

int prepare(VideoState **ps) {
	VideoState *is = *ps;

	if (is->prepare_sync) {
		return -EALREADY;
	}
	is->prepare_sync = 1;
	int ret = prepareAsync_l(ps);
	if (ret != NO_ERROR) {
		return ret;
	}
	if (is->prepare_sync) {
		while (!is->prepared) {
			sleep(1);
		}

		is->prepare_sync = 0;
	}
	return is->prepare_sync;
}

int prepareAsync(VideoState **ps) {
  return prepareAsync_l(ps);
}

int start(VideoState **ps) {
	VideoState *is = *ps;

	if (is && is->audio_player) {
		is->paused = 0;
	    is->player_started = 1;
		setPlayingAudioPlayer(&is->audio_player, 0);
		return NO_ERROR;
	}

	return INVALID_OPERATION;
}

int stop(VideoState **ps) {
	VideoState *is = *ps;

	if (is) {
	    is->quit = 1;
	    /*
	     * If the video has finished playing, then both the picture and
	     * audio queues are waiting for more data.  Make them stop
	     * waiting and terminate normally.
	     */
	    if (is->audioq.initialized == 1) {
	    	SDL_CondSignal(is->audioq.cond);
	    }

	    if (is->videoq.initialized == 1) {
	    	SDL_CondSignal(is->videoq.cond);
	    }

	    if (is->video_refresh_tid) {
	    	pthread_join(*(is->video_refresh_tid), NULL);
	    }

	    if (is->parse_tid) {
	    	pthread_join(*(is->parse_tid), NULL);
	    	printf("one: %d:\n", one);
	    }

	    if (is->video_tid) {
            SDL_CondSignal(is->pictq_cond);
	    	pthread_join(*(is->video_tid), NULL);
	    	printf("two: %d:\n", two);
	    }

        setPlayingAudioPlayer(&is->audio_player, 2);
        
	    clear_l(&is);

		return NO_ERROR;
	}

	return INVALID_OPERATION;
}

int pause_l(VideoState **ps) {
	VideoState *is = *ps;

	if (is && is->audio_player) {
		is->paused = !is->paused;
		setPlayingAudioPlayer(&is->audio_player, 1);
		return NO_ERROR;
	}

	return INVALID_OPERATION;
}

int isPlaying(VideoState **ps) {
	VideoState *is = *ps;

	if (is) {
		if (!is->player_started) {
			return 0;
		} else {
			return !is->paused;
		}
	}

	return 0;
}

int getVideoWidth(VideoState **ps, int *w) {
	VideoState *is = *ps;

	if (!is || !is->video_st) {
		return INVALID_OPERATION;
	}

    *w = is->video_st->codec->width;

    return NO_ERROR;
}

int getVideoHeight(VideoState **ps, int *h) {
	VideoState *is = *ps;

	if (!is || !is->video_st) {
		return INVALID_OPERATION;
	}

    *h = is->video_st->codec->height;

    return NO_ERROR;
}

int seekTo(VideoState **ps, int msec) {
    int result = seekTo_l(ps, msec);
	return result;
}

int getCurrentPosition(VideoState **ps, int *msec) {
	VideoState *is = *ps;

	if (is) {
		*msec = is->audio_clock * 1000;
		return NO_ERROR;
	}

	return INVALID_OPERATION;
}

int getDuration(VideoState **ps, int *msec) {
	return getDuration_l(ps, msec);
}

int reset(VideoState **ps) {
	VideoState *is = *ps;

	if (is) {
	    is->quit = 1;
	    /*
	     * If the video has finished playing, then both the picture and
	     * audio queues are waiting for more data.  Make them stop
	     * waiting and terminate normally.
	     */
	    if (is->audioq.initialized == 1) {
	    	SDL_CondSignal(is->audioq.cond);
	    }

	    if (is->videoq.initialized == 1) {
	    	SDL_CondSignal(is->videoq.cond);
	    }

	    if (is->video_refresh_tid) {
	    	pthread_join(*(is->video_refresh_tid), NULL);
	    }

	    if (is->parse_tid) {
	    	pthread_join(*(is->parse_tid), NULL);
	    }

	    if (is->video_tid) {
            SDL_CondSignal(is->pictq_cond);
	    	pthread_join(*(is->video_tid), NULL);
	    }

	    clear_l(&is);

		return NO_ERROR;
	}

	return INVALID_OPERATION;
}

int setAudioStreamType(VideoState **ps, int type) {
	VideoState *is = *ps;

	if (is) {
		return NO_ERROR;
	}

	return INVALID_OPERATION;

}

int setLooping(VideoState **ps, int loop) {
	VideoState *is = *ps;

	if (is) {
		return NO_ERROR;
	}

	return INVALID_OPERATION;
}

int isLooping(VideoState **ps) {
	VideoState *is = *ps;

	if (is) {
		return NO_ERROR;
	}

	return INVALID_OPERATION;
}

int setVolume(VideoState **ps, float leftVolume, float rightVolume) {
	VideoState *is = *ps;

	if (is && is->audio_player) {
		setVolumeUriAudioPlayer(&is->audio_player, leftVolume);
		return NO_ERROR;
	}

	return INVALID_OPERATION;
}

static Uint32 notify_from_thread_cb(Uint32 interval, void *opaque) {
    Message *message = (Message*) opaque;

    if (!message) {
    	return 0;
    }

    if (message->is && message->is->notify_callback) {
	    message->is->notify_callback(message->is->clazz, message->msg, message->ext1, message->ext2, message->from_thread);
    }

    free(message);

    return 0; /* 0 means stop timer */
}

void notify(VideoState *is, int msg, int ext1, int ext2) {
	if (is->notify_callback) {
		is->notify_callback(is->clazz, msg, ext1, ext2, 0);
	}
}

void notify_from_thread(VideoState *is, int msg, int ext1, int ext2) {
	Message* message = malloc(sizeof(Message));
	message->is = is;
	message->msg = msg;
	message->ext1 = ext1;
	message->ext2 = ext2;
	message->from_thread = 1;

	SDL_AddTimer(0, notify_from_thread_cb, message);
}

int setNextPlayer(VideoState **ps, VideoState *next) {
    /*VideoState *is = *ps;
    
    if (is) {
        is->next = next;
        return NO_ERROR;
    }*/
    
    return NO_ERROR; //INVALID_OPERATION;
}

void clear_l(VideoState **ps) {
	VideoState *is = *ps;

	if (is) {
		if (is->pFormatCtx) {
			avformat_close_input(&is->pFormatCtx);
			is->pFormatCtx = NULL;
		}

		is->videoStream = 0;
		is->audioStream = 0;

		is->av_sync_type = 0;
		is->external_clock = 0;
		is->external_clock_time = 0;
	    is->seek_req = 0;
	    is->seek_flags = 0;
	    is->seek_pos = 0;
	    is->seek_rel = 0;

	    is->audio_clock = 0;
	    is->audio_st = NULL;

	    if (is->audioq.initialized == 1) {
	    	if (is->audioq.first_pkt) {
	    		free(is->audioq.first_pkt);
	    	}

	        if (is->audioq.last_pkt) {
	        	//free(is->audioq.last_pkt);
			}

	        if (is->audioq.mutex) {
	        	free(is->audioq.mutex);
	        	is->audioq.mutex = NULL;
	        }

	        if (is->audioq.cond) {
	        	free(is->audioq.cond);
	        	is->audioq.cond = NULL;
	        }

	        is->audioq.initialized = 0;
	    }

  	    /*AVFrame *frame = &is->audio_frame;
	    if (frame->data) {
	    	av_free_frame(frame);
	    }*/

	    is->audio_buf[0] = '\0';
	    is->audio_buf_size = 0;
	    is->audio_buf_index = 0;

  	    AVPacket *pkt = &is->audio_pkt;
	    if (pkt->data) {
	    	av_packet_unref(pkt);
	    }

	    is->audio_pkt_data = NULL;
	    is->audio_pkt_size = 0;
	    is->audio_hw_buf_size = 0;
	    is->audio_diff_cum = 0;
	    is->audio_diff_avg_coef = 0;
	    is->audio_diff_threshold = 0;
	    is->audio_diff_avg_count = 0;
	    is->frame_timer = 0;
	    is->frame_last_pts = 0;
	    is->frame_last_delay = 0;
	    is->video_clock = 0;
	    is->video_current_pts = 0;
	    is->video_current_pts_time = 0;
	    is->video_st = NULL;

	    if (is->videoq.initialized == 1) {
	    	if (is->videoq.first_pkt) {
	    		free(is->videoq.first_pkt);
	    	}

	        if (is->videoq.last_pkt) {
	        	//free(is->videoq.last_pkt);
			}

	        if (is->videoq.mutex) {
	        	free(is->videoq.mutex);
	        	is->videoq.mutex = NULL;
	        }

	        if (is->videoq.cond) {
	        	free(is->videoq.cond);
	        	is->videoq.cond = NULL;
	        }

	        is->videoq.initialized = 0;
	    }

	    //VideoPicture    pictq[VIDEO_PICTURE_QUEUE_SIZE];
	    is->pictq_size = 0;
	    is->pictq_rindex = 0;
	    is->pictq_windex = 0;

	    if (is->pictq_mutex) {
	    	free(is->pictq_mutex);
	    	is->pictq_mutex = NULL;
	    }

	    if (is->pictq_cond) {
	    	free(is->pictq_cond);
	    	is->pictq_cond = NULL;
	    }

	    if (is->video_refresh_tid) {
	    	free(is->video_refresh_tid);
	    	is->video_refresh_tid = NULL;
	    }

	    if (is->parse_tid) {
	    	free(is->parse_tid);
	    	is->parse_tid = NULL;
	    }

	    if (is->video_tid) {
	    	free(is->video_tid);
	    	is->video_tid = NULL;
	    }

	    //is->filename[0] = '\0';
	    //is->quit = 0;

	    if (is->io_context) {
	    	avio_close(is->io_context);
	    	is->io_context = NULL;
	    }

	    if (is->sws_ctx) {
	    	sws_freeContext(is->sws_ctx);
	    	is->sws_ctx = NULL;
	    }

	    if (is->sws_ctx_audio) {
	    	swr_free(&is->sws_ctx_audio);
	    	is->sws_ctx_audio = NULL;
	    }

	    if (is->audio_player) {
	    	shutdown(&is->audio_player);
	    	is->audio_player = NULL;
	    }

	    //is->audio_callback = NULL;
	    is->prepared = 0;

	    //is->headers[0] = '\0';

	    if (is->fd != -1) {
		    close(is->fd);
        }

	    is->fd = -1;
	    is->offset = 0;

	    is->prepare_sync = 0;

	    //is->notify_callback = NULL;
	    //is->clazz = NULL;

	    is->read_pause_return = 0;

	    is->paused = 0;
	    is->last_paused = -1;
	    is->player_started = 0;

	    av_packet_unref(&is->flush_pkt);
	}
}

int seekTo_l(VideoState **ps, int msec) {
	VideoState *is = *ps;

	if (is) {
		stream_seek(is, msec * 1000, msec * 1000, 0);
		return NO_ERROR;
	}

	return INVALID_OPERATION;
}

int prepareAsync_l(VideoState **ps) {
	VideoState *is = *ps;

    if (is != 0) {
    	is->pictq_mutex = SDL_CreateMutex();
        is->pictq_cond = SDL_CreateCond();

    	is->video_refresh_tid = malloc(sizeof(*(is->video_refresh_tid)));
    	pthread_create(is->video_refresh_tid, NULL, (void *) &video_refresh_timer, is);

    	//schedule_refresh(is, 40);

    	is->av_sync_type = DEFAULT_AV_SYNC_TYPE;
    	is->parse_tid = malloc(sizeof(*(is->parse_tid)));

    	if(!is->parse_tid) {
    	    av_free(is);
    	    return UNKNOWN_ERROR;
    	}

    	pthread_create(is->parse_tid, NULL, (void *) &decode_thread, is);

    	av_init_packet(&is->flush_pkt);
    	is->flush_pkt.data = (unsigned char *)"FLUSH";

    	return NO_ERROR;
    }
    return INVALID_OPERATION;
}

int getDuration_l(VideoState **ps, int *msec) {
	VideoState *is = *ps;

	if (is) {
		if (is->pFormatCtx && (is->pFormatCtx->duration != AV_NOPTS_VALUE)) {
			*msec = (is->pFormatCtx->duration / AV_TIME_BASE) * 1000;
		} else {
			*msec = 0;
		}
		return NO_ERROR;
	}

	return INVALID_OPERATION;
}

int setMetadataFilter(VideoState **ps, char *allow[], char *block[]) {
	return 0;
}

int getMetadata(VideoState **ps, AVDictionary **metadata) {
	return 0;
}

int main(int argc, char *argv[]) {

}

//int main(int argc, char *argv[]) {
//  SDL_Event       event;
//  //double          pts;
//  VideoState      *is;
//
//  is = av_mallocz(sizeof(VideoState));
//
//  if(argc < 2) {
//    fprintf(stderr, "Usage: test <file>\n");
//    exit(1);
//  }
//  // Register all formats and codecs
//  av_register_all();
//
//  av_strlcpy(is->filename, argv[1], 1024);
//
//  setDataSourceURI(&is, "/Users/wseemann/Desktop/BigBuckBunny_320x180.mp4", NULL);
//  prepare(&is);
//  start(&is);
//
//  for(;;) {
//    double incr, pos;
//    SDL_WaitEvent(&event);
//    switch(event.type) {
//    case SDL_KEYDOWN:
//      switch(event.key.keysym.sym) {
//      case SDLK_LEFT:
//	incr = -10.0;
//	goto do_seek;
//      case SDLK_RIGHT:
//	incr = 10.0;
//	goto do_seek;
//      case SDLK_UP:
//	incr = 60.0;
//	goto do_seek;
//      case SDLK_DOWN:
//	incr = -60.0;
//	goto do_seek;
//      do_seek:
//	/*if(global_video_state) {
//	  pos = get_master_clock(global_video_state);
//	  pos += incr;
//	  stream_seek(global_video_state, (int64_t)(pos * AV_TIME_BASE), incr);
//	}*/
//	break;
//      default:
//	break;
//      }
//      break;
//    case FF_QUIT_EVENT:
//    case SDL_QUIT:
//      is->quit = 1;
//      /*
//       * If the video has finished playing, then both the picture and
//       * audio queues are waiting for more data.  Make them stop
//       * waiting and terminate normally.
//       */
//      SDL_CondSignal(is->audioq.cond);
//      SDL_CondSignal(is->videoq.cond);
//      SDL_Quit();
//      exit(0);
//      break;
//    case FF_ALLOC_EVENT:
//      //alloc_picture(event.user.data1);
//      break;
//    case FF_REFRESH_EVENT:
//      video_refresh_timer(event.user.data1);
//      break;
//    default:
//      break;
//    }
//  }
//  return 0;
//}
