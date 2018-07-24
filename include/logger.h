
#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <syslog.h>

void vmixer_logger(const int, const char*, ...);

/** set output to console */
void setConsoleLog(int c);

/** set debug mode logger */
void setDebugMode(int mode);

/** get used debug mode */
int getDebugMode();

/** get usage console */
int getConsoleLog();

#define VMIXER_LOGGER(M, LEVEL, ...) vmixer_logger(LEVEL, "%s:%d: " M, __FILE__, __LINE__, ##__VA_ARGS__)

/**send message to ERROR log level*/
#define ERROR(M, ...) VMIXER_LOGGER(M, LOG_ERR, ##__VA_ARGS__)

/**send message to WARN log level*/
#define WARN(M, ...) VMIXER_LOGGER(M, LOG_WARNING, ##__VA_ARGS__)

/**send message to INFO log level*/
#define INFO(M, ...) VMIXER_LOGGER(M, LOG_INFO, ##__VA_ARGS__)

/**send message to ALERT log level*/
#define ALERT(M, ...) VMIXER_LOGGER(M, LOG_ALERT, ##__VA_ARGS__)

/**send message to DEBUG log level*/
#define DEBUG(M, ...) VMIXER_LOGGER(M, LOG_DEBUG, ##__VA_ARGS__)

/**declare console color back*/
#define BLACK "\033[22;30m"

/**declare console color red*/
#define RED "\033[22;31m"

/**declare console color green*/
#define GREEN "\033[22;32m"

/**declare console color brown*/
#define BROWN "\033[22;33m"

/**declare console color blue*/
#define BLUE "\033[22;34m"

/**declare console color magenta*/
#define MAGENTA "\033[22;35m"

/**declare console color cyan*/
#define CYAN "\033[22;36m"

/**declare console color grey*/
#define GREY "\033[22;37m"

/**declare console color dark_grey*/
#define DARK_GREY "\033[01;30m"

/**declare console color light_red*/
#define LIGHT_RED "\033[01;31m"

/**declare console color light_screen*/
#define LIGHT_SCREEN "\033[01;32m"

/**declare console color yellow*/
#define YELLOW "\033[01;33m"

/**declare console color light_blue*/
#define LIGHT_BLUE "\033[01;34m"

/**declare console color light_magenta*/
#define LIGHT_MAGENTA "\033[01;35m"

/**declare console color light_cyan*/
#define LIGHT_CYAN "\033[01;36m"

/**declare console color white*/
#define WHITE "\033[01;37m"

/**declare end color terminator*/
#define END_COLOR "\033[0m\n"

#endif // LOGGER_H
