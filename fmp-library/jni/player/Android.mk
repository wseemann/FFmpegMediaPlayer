LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := ffmpeg_mediaplayer_jni
LOCAL_CFLAGS := 
LOCAL_SRC_FILES := wseemann_media_MediaPlayer.c \
	JNIHelper.c \
	jni_utils.c
LOCAL_SHARED_LIBRARIES := libavcodec libavformat libavutil
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../ffmpeg/ffmpeg/$(TARGET_ARCH_ABI)/include
LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)
