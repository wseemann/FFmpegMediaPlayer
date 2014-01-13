
#ifndef ANDROIDLOG_H
#define ANDROIDLOG_H

#include <android/log.h>

/* Logging macros.
 *
 * Logs an exception.  If the exception is omitted or NULL, logs the current exception
 * from the JNI environment, if any.
 */
#define LOG_EX(env, priority, tag, ...) \
    IF_LOG(priority, tag) jniLogException(env, ANDROID_##priority, tag, ##__VA_ARGS__)
#define LOGV_EX(env, ...) LOG_EX(env, LOG_VERBOSE, LOG_TAG, ##__VA_ARGS__)
#define LOGD_EX(env, ...) LOG_EX(env, LOG_DEBUG, LOG_TAG, ##__VA_ARGS__)
#define LOGI_EX(env, ...) LOG_EX(env, LOG_INFO, LOG_TAG, ##__VA_ARGS__)
#define LOGW_EX(env, ...) LOG_EX(env, LOG_WARN, LOG_TAG, ##__VA_ARGS__)
#define LOGE_EX(env, ...) LOG_EX(env, LOG_ERROR, LOG_TAG, ##__VA_ARGS__)

#define  LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#endif  /* ANDROIDLOG_H */
