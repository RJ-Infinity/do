#if defined(ANDROID)
#include <android/log.h>

#define LOG_TAG "RJ_TODO_APP"

#define LOG(priority, fmt, ...) ((void)__android_log_print((priority), (LOG_TAG), (fmt)__VA_OPT__(, ) __VA_ARGS__))

#define LOG_F(fmt, ...) LOG(ANDROID_LOG_FATAL, (fmt)__VA_OPT__(, ) __VA_ARGS__)
#define LOG_E(fmt, ...) LOG(ANDROID_LOG_ERROR, (fmt)__VA_OPT__(, ) __VA_ARGS__)
#define LOG_W(fmt, ...) LOG(ANDROID_LOG_WARN, (fmt)__VA_OPT__(, ) __VA_ARGS__)
#define LOG_I(fmt, ...) LOG(ANDROID_LOG_INFO, (fmt)__VA_OPT__(, ) __VA_ARGS__)

#define printf LOG_I
#elif defined(_WIN32)

#define LOG()

// note the lack of parenthisis around fmt this is so the log level will combine
#define LOG_F(fmt, ...) fprintf(stderr, "FATAL: "fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_E(fmt, ...) fprintf(stderr, "ERROR: "fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_W(fmt, ...) fprintf(stderr, "WARN: "fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_I(fmt, ...) fprintf(stderr, "INFO: "fmt __VA_OPT__(, ) __VA_ARGS__)

#endif