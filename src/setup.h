#ifndef __SETUP_H__
#define __SETUP_H__

#include "aliases.h"

#include <windows.h>

typedef union {
  struct {
    // SkyCameraProp functions.
    void *fn_SkyCamera__updateParams;
    void *fn_SkyCamera_updateUI;
    void *fn_SkyCamera_setState;
    void *fn_SkyCamera_update;

    // Player functions.
    void *fn_Player_getCameraPos;

    // Level functions.
    void *fn_Level_interactionTest;

    // WhiskerCamera functions.
    void *fn_WhiskerCamera_update;
  };

  // Array of functions.
  void *functions[7];
} SetupFunctions_t;

i08 setupFuncWithSig(SetupFunctions_t *functions);
i08 setupPaths(HMODULE hModule);

#endif
