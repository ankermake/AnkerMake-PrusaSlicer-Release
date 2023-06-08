#ifndef __CC_GLOBAL_LOG__
#define __CC_GLOBAL_LOG__

// log level
// cxlog_verbose = 0,
// cxlog_debug = 1,
// cxlog_info = 2,
// cxlog_warn = 3,
// cxlog_err = 4,
// cxlog_critical = 5
// cxlog_main = 6

#if CC_USE_SPDLOG
#include "spdlog/aklog_macro.h"


	#define LOGV(...) AKLogVerbose(__VA_ARGS__)
	#define LOGVID(logSortId,...) AKLogVerboseID(logSortId,##__VA_ARGS__)
	#define LOGD(...) AKLogDebug(__VA_ARGS__)
	#define LOGDID(logSortId,...) AKLogDebugID(logSortId,##__VA_ARGS__)
	#define LOGI(...) AKLogInfo(__VA_ARGS__)
	#define LOGIID(logSortId,...) AKLogInfoID(logSortId,##__VA_ARGS__)
	#define LOGW(...) AKLogWarn(__VA_ARGS__)
    #define LOGWID(logSortId,...) AKLogWarnID(logSortId,##__VA_ARGS__)
	#define LOGE(...) AKLogError(__VA_ARGS__)
    #define LOGEID(logSortId,...) AKLogErrorID(logSortId,##__VA_ARGS__)
	#define LOGC(...) AKLogCritical(__VA_ARGS__)
	#define LOGCID(logSortId,...) AKLogCriticalID(logSortId,##__VA_ARGS__)
	#define LOGM(...) AKLogMain(__VA_ARGS__)
	#define LOGMID(logSortId,...) AKLogMainID(logSortId,##__VA_ARGS__)
	
    #define LOGINIT(x) cxlog::AKLog::Instance().InitAKLog(x)
	#define LOGDIR(x) cxlog::AKLog::Instance().setDirectory(x)
	#define LOGLEVEL(x) cxlog::AKLog::Instance().SetLevel(x)
	#define LOGEND()  cxlog::AKLog::Instance().EndLog()
    #define LOGCONSOLE() cxlog::AKLog::Instance().setColorConsole()
	#define LOGNAMEFUNC(x) cxlog::AKLog::Instance().setNameFunc(x)
#else
	#if __ANDROID__
		#include <android/log.h>
		 
		#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE,"NativeCC",__VA_ARGS__)
		#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,"NativeCC",__VA_ARGS__)
		#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"NativeCC",__VA_ARGS__)
		#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,"NativeCC",__VA_ARGS__)
		#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"NativeCC",__VA_ARGS__)
		#define LOGC(...) __android_log_print(ANDROID_LOG_ASSERT,"NativeCC",__VA_ARGS__)
		#define LOGM(...) __android_log_print(ANDROID_LOG_ERROR,"NativeCC",__VA_ARGS__)
	#else
		#include <stdarg.h>
		#include <stdio.h>
		#include <iostream>
		
		#define LOGV(...) printf(__VA_ARGS__);printf("\n")
		#define LOGD(...) printf(__VA_ARGS__);printf("\n")
		#define LOGI(...) printf(__VA_ARGS__);printf("\n")
		#define LOGW(...) printf(__VA_ARGS__);printf("\n")
		#define LOGE(...) printf(__VA_ARGS__);printf("\n")
		#define LOGC(...) printf(__VA_ARGS__);printf("\n")
		#define LOGM(...) printf(__VA_ARGS__);printf("\n")
	#endif

	#define LOGVID(logSortId,...) (void)0
	#define LOGDID(logSortId,...) (void)0
	#define LOGIID(logSortId,...) (void)0
	#define LOGWID(logSortId,...) (void)0
	#define LOGEID(logSortId,...) (void)0
	#define LOGCID(logSortId,...) (void)0
	#define LOGMID(logSortId,...) (void)0

	#define LOGDIR(x) (void)0 
	#define LOGLEVEL(x) (void)0
	#define LOGEND() (void)0
    #define LOGCONSOLE() (void)0
	#define LOGNAMEFUNC(x) (void)0
#endif

#endif // __CC_GLOBAL_LOG__