#ifdef ANDROID
#include <android/log.h>

#define LOG_TAG "RJ_TODO_APP"

#define LOG(priority, fmt, ...) ((void)__android_log_print((priority), (LOG_TAG), (fmt)__VA_OPT__(, ) __VA_ARGS__))

#define LOG_E(fmt, ...) LOG(ANDROID_LOG_ERROR, (fmt)__VA_OPT__(, ) __VA_ARGS__)
#define LOG_W(fmt, ...) LOG(ANDROID_LOG_WARN, (fmt)__VA_OPT__(, ) __VA_ARGS__)
#define LOG_I(fmt, ...) LOG(ANDROID_LOG_INFO, (fmt)__VA_OPT__(, ) __VA_ARGS__)

#define printf LOG_I
#endif