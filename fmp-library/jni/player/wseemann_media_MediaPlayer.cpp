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

//#define LOG_NDEBUG 0
#define LOG_TAG "FFmpegMediaPlayer-JNI"

#include "android/log.h"
#include <mediaplayer.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "jni.h"
#include "Errors.h"  // for int

extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
}

// ----------------------------------------------------------------------------

using namespace std;

// ----------------------------------------------------------------------------

struct fields_t {
    jfieldID    context;
    jfieldID    buffer;
    jfieldID    surface_texture;

    jmethodID   post_event;
    jmethodID   init_audio_track;
    jmethodID   write_audio;
    jmethodID   set_volume;
    jmethodID   attach_aux_effect;
    jmethodID   set_aux_effect_send_level;
};
static fields_t fields;

static JavaVM *m_vm;
//static Mutex sLock;

// ----------------------------------------------------------------------------
// ref-counted object for callbacks
class JNIMediaPlayerListener: public MediaPlayerListener
{
public:
    JNIMediaPlayerListener(JNIEnv* env, jobject thiz, jobject weak_thiz, jbyteArray buffer);
    ~JNIMediaPlayerListener();
    virtual void notify(int msg, int ext1, int ext2, int fromThread);
    //virtual void notify(int msg, int ext1, int ext2, const Parcel *obj = NULL);
    virtual int initAudioTrack(int streamType, int sampleRateInHz, int channelConfig, int sessionId, int fromThread);
    virtual void writeAudio(int16_t *samples, int frameSizePtr, int fromThread);
    virtual int setVolume(float leftVolume, float rightVolume);
    virtual int attachAuxEffect(int effectId);
    virtual int setAuxEffectSendLevel(float level);
private:
    JNIMediaPlayerListener();
    jclass      mClass;     // Reference to MediaPlayer class
    jobject     mObject;    // Weak ref to MediaPlayer Java object to call on
    jobject     mThiz;
    jbyteArray  mAudioFrameRef;
    jbyte*      mAudioFrameRefBuffer;
};

void jniThrowException(JNIEnv* env, const char* className,
    const char* msg) {
    jclass exception = env->FindClass(className);
    env->ThrowNew(exception, msg);
}

JNIMediaPlayerListener::JNIMediaPlayerListener(JNIEnv* env, jobject thiz, jobject weak_thiz, jbyteArray buffer)
{

    // Hold onto the MediaPlayer class for use in calling the static method
    // that posts events to the application thread.
    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == NULL) {
    	__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Can't find wseemann/media/FFmpegMediaPlayer");
        jniThrowException(env, "java/lang/Exception", NULL);
        return;
    }
    mClass = (jclass)env->NewGlobalRef(clazz);
    mThiz = (jobject)env->NewGlobalRef(thiz);

    // We use a weak reference so the MediaPlayer object can be garbage collected.
    // The reference is only used as a proxy for callbacks.
    mObject  = env->NewGlobalRef(weak_thiz);

    //audio frame buffer
    mAudioFrameRef = (jbyteArray) env->NewGlobalRef(buffer); //lock the array preventing the garbage collector from destructing it

    mAudioFrameRefBuffer = env->GetByteArrayElements(mAudioFrameRef, NULL);
}

JNIMediaPlayerListener::~JNIMediaPlayerListener()
{
    // remove global references
    //JNIEnv *env = AndroidRuntime::getJNIEnv();
	JNIEnv *env = 0;
	m_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
    env->DeleteGlobalRef(mObject);
    env->DeleteGlobalRef(mClass);
    env->DeleteGlobalRef(mThiz);
    env->ReleaseByteArrayElements(mAudioFrameRef, mAudioFrameRefBuffer, JNI_ABORT);
    env->DeleteGlobalRef(mAudioFrameRef);
}

void JNIMediaPlayerListener::notify(int msg, int ext1, int ext2, int fromThread)
//void JNIMediaPlayerListener::notify(int msg, int ext1, int ext2, const Parcel *obj)
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "notify: %d", msg);
	JNIEnv *env = 0;
	int isAttached = 0;
    	
	int status  = m_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
		
	if (fromThread) {
		jclass *interface_class;
				
		isAttached = 0;

		if (m_vm->AttachCurrentThread(&env, NULL) < 0) {
			__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "failed to attach current thread");
		}
		
		isAttached = 1;
	}
	
    //JNIEnv *env = AndroidRuntime::getJNIEnv();
    /*if (obj && obj->dataSize() > 0) {
        jobject jParcel = createJavaParcelObject(env);
        if (jParcel != NULL) {
            Parcel* nativeParcel = parcelForJavaObject(env, jParcel);
            nativeParcel->setData(obj->data(), obj->dataSize());
            env->CallStaticVoidMethod(mClass, fields.post_event, mObject,
                    msg, ext1, ext2, jParcel);
            env->DeleteLocalRef(jParcel);
        }
    } else {*/
        env->CallStaticVoidMethod(mClass, fields.post_event, mObject,
                msg, ext1, ext2, NULL);
    //}
        
    if (env->ExceptionCheck()) {
    	__android_log_print(ANDROID_LOG_WARN, LOG_TAG, "An exception occurred while notifying an event.");
        //LOGW_EX(env);
        env->ExceptionClear();
    }
    
    if (fromThread && isAttached) {
    	m_vm->DetachCurrentThread();
    }    
}

int JNIMediaPlayerListener::initAudioTrack(int streamType, int sampleRateInHz, int channelConfig, int sessionId, int fromThread)
{
	JNIEnv *env = 0;
	int isAttached = 0;
	
	int status  = m_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	
	if (fromThread) {
		jclass *interface_class;
			
		isAttached = 0;

		if (m_vm->AttachCurrentThread(&env, NULL) < 0) {
			__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "failed to attach current thread");
		}
		
		isAttached = 1;
	}
	
	int bufferSize = env->CallIntMethod(mThiz, fields.init_audio_track, streamType, sampleRateInHz, channelConfig, sessionId);
	
    if (env->ExceptionCheck()) {
    	__android_log_print(ANDROID_LOG_WARN, LOG_TAG, "An exception occurred while notifying an event.");
        //LOGW_EX(env);
        env->ExceptionClear();
    }
    
    if (fromThread && isAttached) {
    	m_vm->DetachCurrentThread();
    }
    
    return bufferSize;
}

void JNIMediaPlayerListener::writeAudio(int16_t *samples, int frameSizePtr, int fromThread)
{
	JNIEnv *env = 0;
	int isAttached = 0;
	
	int status  = m_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	
	if (fromThread) {
		jclass *interface_class;
			
		isAttached = 0;

		if (m_vm->AttachCurrentThread(&env, NULL) < 0) {
			__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "failed to attach current thread");
		}
		
		isAttached = 1;
	}
	
	memcpy(mAudioFrameRefBuffer, samples, frameSizePtr);
	env->CallVoidMethod(mThiz, fields.write_audio, frameSizePtr);
	
    if (env->ExceptionCheck()) {
    	__android_log_print(ANDROID_LOG_WARN, LOG_TAG, "An exception occurred while notifying an event.");
        //LOGW_EX(env);
        env->ExceptionClear();
    }
    
    if (fromThread && isAttached) {
    	m_vm->DetachCurrentThread();
    }
}

int JNIMediaPlayerListener::setVolume(float leftVolume, float rightVolume)
{
	JNIEnv *env = 0;
	int status  = m_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	
	int ret = -3;
	
	ret = env->CallIntMethod(mThiz, fields.set_volume, leftVolume, rightVolume);
	
	if (env->ExceptionCheck()) {
    	__android_log_print(ANDROID_LOG_WARN, LOG_TAG, "An exception occurred while notifying an event.");
        //LOGW_EX(env);
        env->ExceptionClear();
    }
	
    return ret;
}

int JNIMediaPlayerListener::attachAuxEffect(int effectId)
{
	JNIEnv *env = 0;
	int status  = m_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	
	int ret = -3;
	
	ret = env->CallIntMethod(mThiz, fields.attach_aux_effect, effectId);
	
    if (env->ExceptionCheck()) {
    	__android_log_print(ANDROID_LOG_WARN, LOG_TAG, "An exception occurred while notifying an event.");
        //LOGW_EX(env);
        env->ExceptionClear();
    }
    
    return ret;
}

int JNIMediaPlayerListener::setAuxEffectSendLevel(float level)
{
	JNIEnv *env = 0;
	int status  = m_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	
	int ret = -3;
		
	ret = env->CallIntMethod(mThiz, fields.set_aux_effect_send_level, level);
	
    if (env->ExceptionCheck()) {
    	__android_log_print(ANDROID_LOG_WARN, LOG_TAG, "An exception occurred while notifying an event.");
        //LOGW_EX(env);
        env->ExceptionClear();
    }
    
    return ret;
}

// ----------------------------------------------------------------------------

static MediaPlayer* getMediaPlayer(JNIEnv* env, jobject thiz)
{
    //Mutex::Autolock l(sLock);
    MediaPlayer* const p = (MediaPlayer*)env->GetIntField(thiz, fields.context);
    return p;
}

static MediaPlayer* setMediaPlayer(JNIEnv* env, jobject thiz, int mp)
{
    //Mutex::Autolock l(sLock);
    MediaPlayer* old = (MediaPlayer*)env->GetIntField(thiz, fields.context);
    /*if (player.get()) {
        player->incStrong((void*)setMediaPlayer);
    }
    if (old != 0) {
        old->decStrong((void*)setMediaPlayer);
    }*/
    env->SetIntField(thiz, fields.context, mp);
    return old;
}

// If exception is NULL and opStatus is not OK, this method sends an error
// event to the client application; otherwise, if exception is not NULL and
// opStatus is not OK, this method throws the given exception to the client
// application.
static void process_media_player_call(JNIEnv *env, jobject thiz, int opStatus, const char* exception, const char *message)
{
    if (exception == NULL) {  // Don't throw exception. Instead, send an event.
        if (opStatus != (int) OK) {
            MediaPlayer* mp = getMediaPlayer(env, thiz);
            if (mp != 0) mp->notify(MEDIA_ERROR, opStatus, 0, 0);
        }
    } else {  // Throw exception!
        if ( opStatus == (int) INVALID_OPERATION ) {
            jniThrowException(env, "java/lang/IllegalStateException", NULL);
        } else if ( opStatus == (int) PERMISSION_DENIED ) {
            jniThrowException(env, "java/lang/SecurityException", NULL);
        } else if ( opStatus != (int) OK ) {
            if (strlen(message) > 230) {
               // if the message is too long, don't bother displaying the status code
               jniThrowException( env, exception, message);
            } else {
               char msg[256];
                // append the status code to the message
               sprintf(msg, "%s: status=0x%X", message, opStatus);
               jniThrowException( env, exception, msg);
            }
        }
    }
}

static void
wseemann_media_FFmpegMediaPlayer_setDataSource(
        JNIEnv *env, jobject thiz, jstring path) {

    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    if (path == NULL) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return;
    }

    const char *tmp = env->GetStringUTFChars(path, NULL);
    if (tmp == NULL) {  // Out of memory
        return;
    }
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "setDataSource: path %s", tmp);

    status_t opStatus =
        mp->setDataSource(tmp);

    process_media_player_call(
            env, thiz, opStatus, "java/io/IOException",
            "setDataSource failed." );
    
    env->ReleaseStringUTFChars(path, tmp);
    tmp = NULL;
}

static void
decVideoSurfaceRef(JNIEnv *env, jobject thiz)
{
    /*sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        return;
    }

    sp<IGraphicBufferProducer> old_st = getVideoSurfaceTexture(env, thiz);
    if (old_st != NULL) {
        old_st->decStrong((void*)decVideoSurfaceRef);
    }*/
}

static void
setVideoSurface(JNIEnv *env, jobject thiz, jobject jsurface, jboolean mediaPlayerMustBeAlive)
{
    /*sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        if (mediaPlayerMustBeAlive) {
            jniThrowException(env, "java/lang/IllegalStateException", NULL);
        }
        return;
    }

    decVideoSurfaceRef(env, thiz);

    sp<IGraphicBufferProducer> new_st;
    if (jsurface) {
        sp<Surface> surface(android_view_Surface_getSurface(env, jsurface));
        if (surface != NULL) {
            new_st = surface->getIGraphicBufferProducer();
            if (new_st == NULL) {
                jniThrowException(env, "java/lang/IllegalArgumentException",
                    "The surface does not have a binding SurfaceTexture!");
                return;
            }
            new_st->incStrong((void*)decVideoSurfaceRef);
        } else {
            jniThrowException(env, "java/lang/IllegalArgumentException",
                    "The surface has been released");
            return;
        }
    }

    env->SetIntField(thiz, fields.surface_texture, (int)new_st.get());

    // This will fail if the media player has not been initialized yet. This
    // can be the case if setDisplay() on MediaPlayer.java has been called
    // before setDataSource(). The redundant call to setVideoSurfaceTexture()
    // in prepare/prepareAsync covers for this case.
    mp->setVideoSurfaceTexture(new_st);*/
}

static void
wseemann_media_FFmpegMediaPlayer_setVideoSurface(JNIEnv *env, jobject thiz, jobject jsurface)
{
    setVideoSurface(env, thiz, jsurface, true /* mediaPlayerMustBeAlive */);
}

static void
wseemann_media_FFmpegMediaPlayer_prepare(JNIEnv *env, jobject thiz)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    // Handle the case where the display surface was set before the mp was
    // initialized. We try again to make it stick.
    //sp<IGraphicBufferProducer> st = getVideoSurfaceTexture(env, thiz);
    //mp->setVideoSurfaceTexture(st);

    process_media_player_call( env, thiz, mp->prepare(), "java/io/IOException", "Prepare failed." );
}

static void
wseemann_media_FFmpegMediaPlayer_prepareAsync(JNIEnv *env, jobject thiz)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    // Handle the case where the display surface was set before the mp was
    // initialized. We try again to make it stick.
    //sp<IGraphicBufferProducer> st = getVideoSurfaceTexture(env, thiz);
    //mp->setVideoSurfaceTexture(st);

    process_media_player_call( env, thiz, mp->prepareAsync(), "java/io/IOException", "Prepare Async failed." );
}

static void
wseemann_media_FFmpegMediaPlayer_start(JNIEnv *env, jobject thiz)
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "start");
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->start(), NULL, NULL );
}

static void
wseemann_media_FFmpegMediaPlayer_stop(JNIEnv *env, jobject thiz)
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "stop");
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->stop(), NULL, NULL );
}

static void
wseemann_media_FFmpegMediaPlayer_pause(JNIEnv *env, jobject thiz)
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "pause");
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->pause(), NULL, NULL );
}

static jboolean
wseemann_media_FFmpegMediaPlayer_isPlaying(JNIEnv *env, jobject thiz)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }
    const jboolean is_playing = mp->isPlaying();

    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "isPlaying: %d", is_playing);
    return is_playing;
}

static void
wseemann_media_FFmpegMediaPlayer_seekTo(JNIEnv *env, jobject thiz, int msec)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "seekTo: %d(msec)", msec);
    process_media_player_call( env, thiz, mp->seekTo(msec), NULL, NULL );
}

static int
wseemann_media_FFmpegMediaPlayer_getVideoWidth(JNIEnv *env, jobject thiz)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int w;
    if (0 != mp->getVideoWidth(&w)) {
    	__android_log_write(ANDROID_LOG_ERROR, LOG_TAG, "getVideoWidth failed");
        w = 0;
    }
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "getVideoWidth: %d", w);
    return w;
}

static int
wseemann_media_FFmpegMediaPlayer_getVideoHeight(JNIEnv *env, jobject thiz)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int h;
    if (0 != mp->getVideoHeight(&h)) {
    	__android_log_write(ANDROID_LOG_ERROR, LOG_TAG, "getVideoHeight failed");
        h = 0;
    }
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "getVideoHeight: %d", h);
    return h;
}


static int
wseemann_media_FFmpegMediaPlayer_getCurrentPosition(JNIEnv *env, jobject thiz)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int msec;
    process_media_player_call( env, thiz, mp->getCurrentPosition(&msec), NULL, NULL );
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "getCurrentPosition: %d (msec)", msec);
    return msec;
}

static int
wseemann_media_FFmpegMediaPlayer_getDuration(JNIEnv *env, jobject thiz)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int msec;
    process_media_player_call( env, thiz, mp->getDuration(&msec), NULL, NULL );
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "getDuration: %d (msec)", msec);
    return msec;
}

static void
wseemann_media_FFmpegMediaPlayer_reset(JNIEnv *env, jobject thiz)
{
	__android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "reset");
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->reset(), NULL, NULL );
}

static void
wseemann_media_FFmpegMediaPlayer_setAudioStreamType(JNIEnv *env, jobject thiz, int streamtype)
{
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "setAudioStreamType: %d", streamtype);
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->setAudioStreamType(streamtype) , NULL, NULL );
}

static void
wseemann_media_FFmpegMediaPlayer_setLooping(JNIEnv *env, jobject thiz, jboolean looping)
{
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "setLooping: %d", looping);
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->setLooping(looping), NULL, NULL );
}

static jboolean
wseemann_media_FFmpegMediaPlayer_isLooping(JNIEnv *env, jobject thiz)
{
    __android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "isLooping");
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }
    return mp->isLooping();
}

static void
wseemann_media_FFmpegMediaPlayer_setVolume(JNIEnv *env, jobject thiz, float leftVolume, float rightVolume)
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "setVolume: left %f  right %f", leftVolume, rightVolume);
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->setVolume(leftVolume, rightVolume), NULL, NULL );
}

// Sends the new filter to the client.
static jint
wseemann_media_FFmpegMediaPlayer_setMetadataFilter(JNIEnv *env, jobject thiz, jobjectArray allow, jobjectArray block)
{
    MediaPlayer* media_player = getMediaPlayer(env, thiz);
    if (media_player == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return UNKNOWN_ERROR;
    }

    int allowCount = env->GetArrayLength(allow);
    char * allowed[allowCount];
    
    for (int i = 0; i < allowCount; i++) {
        jstring allowString = (jstring) env->GetObjectArrayElement(allow, i);
        const char *rawAllowString = env->GetStringUTFChars(allowString, 0);
        //strcpy(allowed[i], rawAllowString);
        //env->ReleaseStringUTFChars(allowString, rawAllowString);
    }
    
    int blockCount = env->GetArrayLength(block);
    char * blocked[blockCount];
    
    for (int i = 0; i < blockCount; i++) {
        jstring blockString = (jstring) env->GetObjectArrayElement(block, i);
        const char *rawBlockString = env->GetStringUTFChars(blockString, 0);
        //strcpy(blocked[i], rawBlockString);
        //env->ReleaseStringUTFChars(blockString, rawBlockString);
    }
    
    //return media_player->setMetadataFilter(allowed, blocked);
    return 0;
}

static jobject
wseemann_media_FFmpegMediaPlayer_getMetadata(JNIEnv *env, jobject thiz)
{
    MediaPlayer* media_player = getMediaPlayer(env, thiz);
    if (media_player == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return NULL;
    }

    char *value;
    int i;
    const char * metadata_keys[25];
    
    metadata_keys[0] = "album";
    metadata_keys[1] = "album_artist";
    metadata_keys[2] = "artist";
    metadata_keys[3] = "comment";
    metadata_keys[4] = "composer";
    metadata_keys[5] = "copyright";
    metadata_keys[6] = "creation_time";
    metadata_keys[7] = "date";
    metadata_keys[8] = "disc";
    metadata_keys[9] = "encoder";
    metadata_keys[10] = "encoded_by";
    metadata_keys[11] = "filename";
    metadata_keys[12] = "genre";
    metadata_keys[13] = "language";
    metadata_keys[14] = "performer";
    metadata_keys[15] = "publisher";
    metadata_keys[16] = "service_name";
    metadata_keys[17] = "service_provider";
    metadata_keys[18] = "title";
    metadata_keys[19] = "track";
    metadata_keys[20] = "bitrate";
    metadata_keys[21] = "duration";
    metadata_keys[22] = "audio_codec";
    metadata_keys[23] = "video_codec";
    metadata_keys[24] = "rotation";

    jclass hashMap_Clazz = env->FindClass("java/util/HashMap");
    jmethodID gHashMap_initMethodID = env->GetMethodID(hashMap_Clazz, "<init>", "()V");
    jobject hashmap = env->NewObject(hashMap_Clazz, gHashMap_initMethodID);
    jmethodID gHashMap_putMethodID = env->GetMethodID(hashMap_Clazz, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    for (i = 0; i < 25; i++) {
    	value = NULL;
    	media_player->getMetadata(metadata_keys[i], &value);

    	if (value) {
    		jstring jKey = env->NewStringUTF(metadata_keys[i]);
    		jstring jValue = env->NewStringUTF(value);

    		(jobject) env->CallObjectMethod(hashmap, gHashMap_putMethodID, jKey, jValue);
    		
    		env->DeleteLocalRef(jKey);
    		env->DeleteLocalRef(jValue);
    	}
    }
    
    return hashmap;
}

// This function gets some field IDs, which in turn causes class initialization.
// It is called from a static block in MediaPlayer, which won't run until the
// first time an instance of this class is used.
static void
wseemann_media_FFmpegMediaPlayer_native_init(JNIEnv *env)
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "native_init");
    jclass clazz;

    clazz = env->FindClass("wseemann/media/FFmpegMediaPlayer");
    if (clazz == NULL) {
        return;
    }

    fields.context = env->GetFieldID(clazz, "mNativeContext", "I");
    if (fields.context == NULL) {
        return;
    }

    fields.buffer = env->GetFieldID(clazz, "mBuffer", "[B");
    if (fields.buffer == NULL) {
        return;
    }
    
    fields.post_event = env->GetStaticMethodID(clazz, "postEventFromNative",
                                               "(Ljava/lang/Object;IIILjava/lang/Object;)V");
    if (fields.post_event == NULL) {
        return;
    }

    fields.init_audio_track = env->GetMethodID(clazz, "initAudioTrack", "(IIII)I");
    if (fields.init_audio_track == NULL) {
        return;
    }
    
    fields.write_audio = env->GetMethodID(clazz, "writeAudio", "(I)V");
    if (fields.write_audio == NULL) {
        return;
    }
    
    fields.set_volume = env->GetMethodID(clazz, "_setVolume", "(FF)I");
    if (fields.set_volume == NULL) {
        return;
    }
    
    fields.attach_aux_effect = env->GetMethodID(clazz, "attachAuxEffectCompat", "(I)I");
    if (fields.attach_aux_effect == NULL) {
        return;
    }
    
    fields.set_aux_effect_send_level = env->GetMethodID(clazz, "setAuxEffectSendLevelCompat", "(F)I");
    if (fields.set_aux_effect_send_level == NULL) {
        return;
    }
    
    fields.surface_texture = env->GetFieldID(clazz, "mNativeSurfaceTexture", "I");
    if (fields.surface_texture == NULL) {
        return;
    }

    // Initialize libavformat and register all the muxers, demuxers and protocols.
    av_register_all();
    avformat_network_init();
}

static void
wseemann_media_FFmpegMediaPlayer_native_setup(JNIEnv *env, jobject thiz, jobject weak_this, jbyteArray buffer)
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "native_setup");
    MediaPlayer* mp = new MediaPlayer();
    if (mp == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
        return;
    }

    // create new listener and give it to MediaPlayer
    JNIMediaPlayerListener* listener = new JNIMediaPlayerListener(env, thiz, weak_this, buffer);
    mp->setListener(listener);

    // Stow our new C++ MediaPlayer in an opaque field in the Java object.
    setMediaPlayer(env, thiz, (int)mp);
}

static void
wseemann_media_FFmpegMediaPlayer_release(JNIEnv *env, jobject thiz)
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "release");
    //decVideoSurfaceRef(env, thiz);
    MediaPlayer* mp = setMediaPlayer(env, thiz, 0);
    if (mp != NULL) {
        // this prevents native callbacks after the object is released
    	JNIMediaPlayerListener *listener = (JNIMediaPlayerListener *) mp->getListener();
    	delete listener;
        mp->setListener(0);
        mp->disconnect();
        
        delete mp;
        setMediaPlayer(env, thiz, 0);
    }
}

static void
wseemann_media_FFmpegMediaPlayer_native_finalize(JNIEnv *env, jobject thiz)
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "native_finalize");
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp != NULL) {
    	__android_log_print(ANDROID_LOG_WARN, LOG_TAG, "MediaPlayer finalized without being released");
    }
    wseemann_media_FFmpegMediaPlayer_release(env, thiz);
}

static void wseemann_media_FFmpegMediaPlayer_set_audio_session_id(JNIEnv *env,  jobject thiz, jint sessionId) {
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "set_session_id(): %d", sessionId);
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->setAudioSessionId(sessionId), NULL, NULL );
}

static jint wseemann_media_FFmpegMediaPlayer_get_audio_session_id(JNIEnv *env,  jobject thiz) {
    __android_log_write(ANDROID_LOG_VERBOSE, LOG_TAG, "get_session_id()");
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }

    return mp->getAudioSessionId();
}

static void
wseemann_media_FFmpegMediaPlayer_setAuxEffectSendLevel(JNIEnv *env, jobject thiz, jfloat level)
{
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "setAuxEffectSendLevel: level %f", level);
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->setAuxEffectSendLevel(level), NULL, NULL );
}

static void wseemann_media_FFmpegMediaPlayer_attachAuxEffect(JNIEnv *env,  jobject thiz, jint effectId) {
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "attachAuxEffect(): %d", effectId);
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->attachAuxEffect(effectId), NULL, NULL );
}

// ----------------------------------------------------------------------------

static JNINativeMethod gMethods[] = {
    {"setDataSource",       "(Ljava/lang/String;)V",            (void *)wseemann_media_FFmpegMediaPlayer_setDataSource},
    {"_setVideoSurface",    "(Landroid/view/Surface;)V",        (void *)wseemann_media_FFmpegMediaPlayer_setVideoSurface},
    {"prepare",             "()V",                              (void *)wseemann_media_FFmpegMediaPlayer_prepare},
    {"prepareAsync",        "()V",                              (void *)wseemann_media_FFmpegMediaPlayer_prepareAsync},
    {"_start",              "()V",                              (void *)wseemann_media_FFmpegMediaPlayer_start},
    {"_stop",               "()V",                              (void *)wseemann_media_FFmpegMediaPlayer_stop},
    {"getVideoWidth",       "()I",                              (void *)wseemann_media_FFmpegMediaPlayer_getVideoWidth},
    {"getVideoHeight",      "()I",                              (void *)wseemann_media_FFmpegMediaPlayer_getVideoHeight},
    {"seekTo",              "(I)V",                             (void *)wseemann_media_FFmpegMediaPlayer_seekTo},
    {"_pause",              "()V",                              (void *)wseemann_media_FFmpegMediaPlayer_pause},
    {"isPlaying",           "()Z",                              (void *)wseemann_media_FFmpegMediaPlayer_isPlaying},
    {"getCurrentPosition",  "()I",                              (void *)wseemann_media_FFmpegMediaPlayer_getCurrentPosition},
    {"getDuration",         "()I",                              (void *)wseemann_media_FFmpegMediaPlayer_getDuration},
    {"_release",            "()V",                              (void *)wseemann_media_FFmpegMediaPlayer_release},
    {"_reset",              "()V",                              (void *)wseemann_media_FFmpegMediaPlayer_reset},
    {"setAudioStreamType",  "(I)V",                             (void *)wseemann_media_FFmpegMediaPlayer_setAudioStreamType},
    {"setLooping",          "(Z)V",                             (void *)wseemann_media_FFmpegMediaPlayer_setLooping},
    {"isLooping",           "()Z",                              (void *)wseemann_media_FFmpegMediaPlayer_isLooping},
    {"setVolume",           "(FF)V",                            (void *)wseemann_media_FFmpegMediaPlayer_setVolume},
    {"native_setMetadataFilter", "([Ljava/lang/String;[Ljava/lang/String;)I", (void *)wseemann_media_FFmpegMediaPlayer_setMetadataFilter},
    {"native_getMetadata",  "()Ljava/util/HashMap;",            (void *)wseemann_media_FFmpegMediaPlayer_getMetadata},
    {"native_init",         "()V",                              (void *)wseemann_media_FFmpegMediaPlayer_native_init},
    {"native_setup",        "(Ljava/lang/Object;[B)V",          (void *)wseemann_media_FFmpegMediaPlayer_native_setup},
    {"native_finalize",     "()V",                              (void *)wseemann_media_FFmpegMediaPlayer_native_finalize},
    {"getAudioSessionId",   "()I",                              (void *)wseemann_media_FFmpegMediaPlayer_get_audio_session_id},
    {"setAudioSessionId",   "(I)V",                             (void *)wseemann_media_FFmpegMediaPlayer_set_audio_session_id},
    {"setAuxEffectSendLevel", "(F)V",                           (void *)wseemann_media_FFmpegMediaPlayer_setAuxEffectSendLevel},
    {"attachAuxEffect",     "(I)V",                             (void *)wseemann_media_FFmpegMediaPlayer_attachAuxEffect},
};

static const char* const kClassPathName = "wseemann/media/FFmpegMediaPlayer";

// This function only registers the native methods
static int register_wseemann_media_FFmpegMediaPlayer(JNIEnv *env)
{
    int numMethods = (sizeof(gMethods) / sizeof( (gMethods)[0]));
    jclass clazz = env->FindClass("wseemann/media/FFmpegMediaPlayer");
    jint ret = env->RegisterNatives(clazz, gMethods, numMethods);
    env->DeleteLocalRef(clazz);
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    m_vm = vm;
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    if (register_wseemann_media_FFmpegMediaPlayer(env) < 0) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "ERROR: FFmpegMediaPlayer native registration failed\n");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_6;

bail:
    return result;
}

// KTHXBYE
