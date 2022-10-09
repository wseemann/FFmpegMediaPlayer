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

#include <audioplayer.h>

void private_audio_callback(void *userdata, Uint8 *stream, int len) {
	VideoState *is = (VideoState *)userdata;
	is->audio_callback(userdata, stream, len);
}

void createEngine(AudioPlayer **ps) {
	AudioPlayer *is = *ps;

	if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		exit(1);
	}

	ps = &is;
}

void createBufferQueueAudioPlayer(AudioPlayer **ps, void *state, int numChannels, int samplesPerSec) {
	SDL_AudioSpec wanted_spec, spec;

	wanted_spec.freq = samplesPerSec;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = numChannels;
	wanted_spec.silence = 0;
	wanted_spec.samples = 1024;
	wanted_spec.callback = private_audio_callback;
	wanted_spec.userdata = state;

    if(SDL_OpenAudio(&wanted_spec, &spec) < 0) {
      fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
      //return -1;
    }
}

void setPlayingAudioPlayer(AudioPlayer **ps, int playstate) {
	SDL_PauseAudio(playstate);
}

void setVolumeUriAudioPlayer(AudioPlayer **ps, int millibel) {

}

void queueAudioSamples(AudioPlayer **ps, void *state)
{
    AudioPlayer *player = *ps;

    //bqPlayerCallback(NULL, state);
}

int enqueue(AudioPlayer **ps, int16_t *data, int size) {
    return 0;
}

void shutdown(AudioPlayer **ps) {
	SDL_Quit();
}
