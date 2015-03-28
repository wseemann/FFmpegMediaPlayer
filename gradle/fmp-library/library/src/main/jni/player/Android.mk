LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := ffmpeg_mediaplayer_jni
LOCAL_CFLAGS := 
LOCAL_SRC_FILES := wseemann_media_MediaPlayer.cpp \
		mediaplayer.cpp \
		ffmpeg_mediaplayer.c \
                player.c
LOCAL_SHARED_LIBRARIES := libswresample libavcodec libavformat libavutil
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../ffmpeg/ffmpeg/$(TARGET_ARCH_ABI)/include
# for native audio
LOCAL_LDLIBS += -lOpenSLES
# for logging
LOCAL_LDLIBS += -llog

include $(BUILD_SHARED_LIBRARY)
