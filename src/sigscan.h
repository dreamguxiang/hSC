#ifndef __SIGSCAN_H__
#define __SIGSCAN_H__

#include "aliases.h"

void *sigScan(const char *moduleName, const char *sig, i32 offset);
void *sigScanE8(const char *moduleName, const char *sig, i32 offset);
i08 sigCheckProcess(void *addr, char *sig);

#endif
