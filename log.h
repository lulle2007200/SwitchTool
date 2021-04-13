#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

typedef enum log_level_e {LOG_LEVEL_FATAL,
						  LOG_LEVEL_ERROR, 
				          LOG_LEVEL_WARNING,
				          LOG_LEVEL_INFO,
				          LOG_LEVEL_DEBUG,
				          LOG_LEVEL_TRACE,
				          LOG_LEVEL_VERBOSE}log_level_t;

typedef struct log_log_event_s{
	void 		*privateData;
	const char 	*file;
	int 		line;
	const char 	*logMsg;
	log_level_t logLevel;
	time_t 		logTime;
	va_list 	vaList;
}log_log_event_t;

typedef void (*log_callback)(const log_log_event_t *logEvent);

void LogSetLogLevel(const log_level_t logLevel);

log_level_t LogGetLogLevel(void);

void LogInit();

bool LogAddCallback(void *const privateData,
                    const log_level_t logLevel,
                    log_callback logCallback);

void LogSetQuiet(bool quiet);

void Log(const log_level_t logLevel,
         const char *const file,
         const int line,
         const char *const logMsg,
         ...);

#define LogFatal(log_msg, ...) \
	Log(LOG_LEVEL_FATAL, __FILE__, __LINE__, log_msg, __VA_ARGS__)
#define LogError(log_msg, ...) \
	Log(LOG_LEVEL_ERROR, __FILE__, __LINE__, log_msg, __VA_ARGS__)
#define LogWarning(log_msg, ...) \
	Log(LOG_LEVEL_WARNING, __FILE__, __LINE__, log_msg, __VA_ARGS__)
#define LogInfo(log_msg, ...) \
	Log(LOG_LEVEL_INFO, __FILE__, __LINE__, log_msg, __VA_ARGS__)
#define LogDebug(log_msg, ...) \
	Log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, log_msg, __VA_ARGS__)
#define LogTrace(log_msg, ...) \
	Log(LOG_LEVEL_TRACE, __FILE__, __LINE__, log_msg, __VA_ARGS__)
#define LogVerbose(log_msg, ...) \
	Log(LOG_LEVEL_VERBOSE, __FILE__, __LINE__, log_msg, __VA_ARGS__)

#endif