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

/*
 * JNI helper functions.
 */
#ifndef _JNIHELPER_H
#define _JNIHELPER_H

#include <jni.h>
#include <android/log.h>

/*
 * Throw an exception with the specified class and an optional message.
 * The "className" argument will be passed directly to FindClass, which
 * takes strings with slashes (e.g. "java/lang/Object").
 *
 * Returns 0 on success, nonzero if something failed (e.g. the exception
 * class couldn't be found).
 *
 * Currently aborts the VM if it can't throw the exception.
 */
int jniThrowException(JNIEnv* env, const char* className, const char* msg);

#endif /*_JNIHELPER_H*/

