#ifndef __HOOKED_H__
#define __HOOKED_H__

#include "aliases.h"
#include "setup.h"

#ifdef __cplusplus
extern "C" {
#endif

i08 initAllHooks();
i08 createAllHooks();
i08 removeAllHooks();

#ifdef __cplusplus
}
#endif

#endif
