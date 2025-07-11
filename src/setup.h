#ifndef __SETUP_H__
#define __SETUP_H__

#include "aliases.h"

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef union {
  struct {
    // SkyCameraProp functions.
    void *fn_SkyCameraProp__updateParams;
    void *fn_SkyCameraProp_updateUI;
    void *fn_SkyCameraProp_setState;
    void *fn_SkyCameraProp_update;

    // Player functions.
    void *fn_Player_getCameraPos;

    // Level functions.
    void *fn_Level_interactionTest;

    // WhiskerCamera functions.
    void *fn_WhiskerCamera_update;

    // SkyCamera functions.
    void *fn_SkyCamera_update;

    // MainCamera functions.
    void *fn_MainCamera__getDelta;
  };

  // Array of functions.
  void *functions[9];
} SetupFunctions_t;

i08 setupFuncWithSig(SetupFunctions_t *functions);
i08 setupPaths(HMODULE hModule, char *prefPath, char *guiIniPath);

#ifdef __cplusplus
}
#endif

#endif
