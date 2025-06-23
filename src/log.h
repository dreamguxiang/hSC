#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>

#define DEBUG_CONSOLE

#ifdef DEBUG_CONSOLE
#define LOGI(format, ...) logiImp(format, ##__VA_ARGS__)
#define WLOGI(format, ...) wlogiImp(format, ##__VA_ARGS__)
#else
#define LOGI(format, ...) /* Nothing. */
#define WLOGI(format, ...) /* Nothing. */
#endif

void recreateConsole();
void logiImp(const char *format, ...);
void wlogiImp(const wchar_t *format, ...);

#endif
