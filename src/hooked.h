#include "aliases.h"

#define OVERRIDE_2(cond, v1, v2) ((cond) ? ((v1) = (v2)) : ((v2) = (v1)))
#define OVERRIDE_3(cond, v11, v12, v2) ((cond) ? ((v11) = ((v12) = (v2))) : ((v2) = (v11)))

extern LPVOID origin_SkyCamera_update
  , origin_SkyCamera_updateUI
  , origin_SkyCamera__updateParams;

i08 createAllHooks(void *baseAddr);
