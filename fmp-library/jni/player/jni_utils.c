/*
 * FFmpegMediaPlayer: A unified interface for playing audio files and streams.
 *
 * Copyright 2013 William Seemann
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android/log.h>
#include <jni.h>
#include <stddef.h>
#include <jni_utils.h>

void attach_to_current_thread(JavaVM *vm, jobject class_path, int *is_attached, JNIEnv **env, jclass *interface_class) {
	*is_attached = 0;

	if ((*vm)->GetEnv(vm, (void**)env, JNI_VERSION_1_6) < 0) {
		if ((*vm)->AttachCurrentThread(vm, (void*)env, NULL) < 0) {
			__android_log_print(ANDROID_LOG_ERROR, "attach_to_current_thread", "failed to attach current thread");
			return;
		}
		*is_attached = 1;
	}

	*interface_class = (**env)->GetObjectClass(*env, class_path);

	if (!*interface_class) {
		__android_log_print(ANDROID_LOG_ERROR, "attach_to_current_thread", "failed to get class reference");
		detach_from_current_thread(vm, *is_attached);
		*is_attached = 0;
	}
}

void detach_from_current_thread(JavaVM *vm, int is_attached) {
	if (is_attached) {
		(*vm)->DetachCurrentThread(vm);
	}
}
