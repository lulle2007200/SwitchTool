#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <Windows.h>

#include "log.h"

#define LOG_MAX_CALLBACKS 10

typedef struct log_callback_s{
	void 			*privateData;
	log_callback	logCallback;
	log_level_t		logLevel;
}log_callback_t;

typedef struct log_state_s{
	log_callback_t	logCallbacks[LOG_MAX_CALLBACKS];
	log_level_t		logLevel;
	uint32_t		nCallbacks;
	bool			quiet;
}log_state_t;

typedef TCHAR log_time_str_t[20];

static log_state_t gLogState = {
	.logLevel = LOG_LEVEL_VERBOSE,
	.logCallbacks = {0},
	.nCallbacks = 0,
	.quiet = true,
};

static const TCHAR *gLogLevelStr[] = {[LOG_LEVEL_FATAL] = TEXT("FATAL"),
						 			  [LOG_LEVEL_ERROR] = TEXT("ERROR"),
						 			  [LOG_LEVEL_WARNING] = TEXT("WARNING"),
						 			  [LOG_LEVEL_INFO] = TEXT("INFO"),
						 			  [LOG_LEVEL_DEBUG] = TEXT("DEBUG"),
						 			  [LOG_LEVEL_TRACE] = TEXT("TRACE"),
						 			  [LOG_LEVEL_VERBOSE] = TEXT("VERBOSE")};


bool LogAddCallback(void *const privateData,
                    const log_level_t logLevel,
                    log_callback logCallback){
	if(gLogState.nCallbacks < LOG_MAX_CALLBACKS){
		gLogState.logCallbacks[gLogState.nCallbacks].logCallback = logCallback;
		gLogState.logCallbacks[gLogState.nCallbacks].privateData = privateData;
		gLogState.logCallbacks[gLogState.nCallbacks].logLevel = logLevel;
		gLogState.nCallbacks++;
		return(true);
	}
	return(false);
}

void LogGetLogTimeStr(log_time_str_t logTimeStr, time_t logTime){
	struct tm tmTime;
	localtime_s(&tmTime, &logTime);
	_tcsftime(logTimeStr,
	         sizeof(log_time_str_t), 
	         TEXT("%d-%m-%Y %H:%M:%S"), &tmTime);
}

void LogDefaultStreamCallback(const log_log_event_t *const logEvent){
	log_time_str_t logTimeStr;
	LogGetLogTimeStr(logTimeStr, 
	                 logEvent->logTime);
	_ftprintf(logEvent->privateData,
	          TEXT("[%s] [%s] %s:%i:"),
	          logTimeStr,
	          gLogLevelStr[logEvent->logLevel],
	          logEvent->file,
	          logEvent->line);
	_vftprintf(logEvent->privateData,
	          logEvent->logMsg,
	          logEvent->vaList);
	_ftprintf(logEvent->privateData,
	          TEXT("\n"));
}

void Log(const log_level_t logLevel,
         const TCHAR *const file,
         const int line,
         const TCHAR *const logMsg,
         ...){
	log_log_event_t logEvent = {
		.file = file,
		.line = line,
		.logLevel = logLevel,
		.logMsg = logMsg,
		.logTime = time(NULL),
	};

	if(!gLogState.quiet && logLevel <= gLogState.logLevel){
		logEvent.privateData = stderr;
		va_start(logEvent.vaList, logMsg);
		LogDefaultStreamCallback(&logEvent);
		va_end(logEvent.vaList);
	}

	for(uint32_t i = 0; i < gLogState.nCallbacks; i++){
		if(logLevel <= gLogState.logCallbacks[i].logLevel){
			logEvent.privateData = gLogState.logCallbacks[i].privateData;
			va_start(logEvent.vaList, logMsg);
			gLogState.logCallbacks[i].logCallback(&logEvent);
			va_end(logEvent.vaList);
		}
	}
}

void LogSetLogLevel(const log_level_t logLevel){
	gLogState.logLevel = logLevel;
}

const TCHAR *LogGetLogLevelStr(log_level_t logLevel){
	return(gLogLevelStr[logLevel]);
}

log_level_t LogGetLogLevel(void){
	return(gLogState.logLevel);
}

void LogSetQuiet(bool quiet){
	gLogState.quiet = quiet;
}