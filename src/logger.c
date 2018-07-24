
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include "logger.h"

int consoleLog = 0;
int debugMode = 0;

void vmixer_logger(const int level, const char* format, ...) {

	if (!debugMode && level == LOG_DEBUG)
		return;

	va_list ap;
	va_start(ap, format);

	if (consoleLog) {
		const char *color_prefix = "";
		switch (level) {

			case LOG_ERR:
				color_prefix = RED;
				break;

			case LOG_ALERT:
				color_prefix = LIGHT_RED;
				break;

			case LOG_WARNING:
				color_prefix = YELLOW;
				break;

			case LOG_INFO:
				color_prefix = WHITE;
				break;

			case LOG_DEBUG:
				color_prefix = GREY;
				break;
		}

		time_t timer = time(NULL);
		struct tm* tm = localtime(&timer);

		fprintf(stderr, "%s%04d-%02d-%02d %02d:%02d:%02d ",
			color_prefix,
			tm->tm_year + 1900,
			tm->tm_mon + 1,
			tm->tm_mday,
			tm->tm_hour,
			tm->tm_min,
			tm->tm_sec
		);

		vfprintf(stderr, format, ap);
		fputs(END_COLOR, stderr);
	}

	else
		vsyslog(level, format, ap);

	va_end(ap);
}

void setConsoleLog(int c) {

	consoleLog = c;
}

void setDebugMode(int mode) {

	debugMode = mode;
}

int getDebugMode() {

	return debugMode;
}

int getConsoleLog() {

	return consoleLog;
}
